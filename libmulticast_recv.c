#include "libmulticast.h"
#include "libmulticast_defs.h"
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

#define MSG_LIBMULTICAST_CLEANUP "e"

struct cb_args {
    void (*fptr)(char*, int);
    char *data;
    int data_len;
};

static void* create_cb_thread(void *args) {
    struct cb_args *cb_args = (struct cb_args*)args;
    if (!args) {
        perror("Args NULL\n");
        return NULL;
    }
    if (!cb_args->fptr || !cb_args->data || cb_args->data_len <= 0) {
        perror("One arg is NULL\n");
        if (cb_args->data) free(cb_args->data);
        free(args);
        return NULL;
    }
    cb_args->fptr(cb_args->data, cb_args->data_len);
    free(cb_args->data);
    free(args);
    return NULL;
}

static int is_running(struct mcreceiver* rc) {
    pthread_mutex_lock(&rc->mutex);
    int ret = rc->running;
    pthread_mutex_unlock(&rc->mutex);
    return ret;
}

static void set_running(struct mcreceiver* rc, int val) {
    pthread_mutex_lock(&rc->mutex);
    rc->running = val;
    pthread_mutex_unlock(&rc->mutex);
}

static void cancel_select(struct mcreceiver* rc) {
    if (write(rc->pipe[1], MSG_LIBMULTICAST_CLEANUP, strlen(MSG_LIBMULTICAST_CLEANUP)) < 0) {
        perror("Error writing to select() canceling pipe.");
    }
}

static int is_select_cancelled(struct mcreceiver* rc) {
    char tmp[2] = {0};
    int r = read(rc->pipe[0], tmp, sizeof(tmp));
    if (r < 0) {
        perror("Error reading from select() canceling pipe.");
    }
    return (r > 0);
}

// Loop thread for listener. 
// This function will return. This function will create an extra thread.
static void _recv(struct mcreceiver* rc) {
    while(is_running(rc)) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(rc->sd, &read_set);
        FD_SET(rc->pipe[0], &read_set);
        FD_SET(rc->pipe[1], &read_set);
        select(FD_SETSIZE, &read_set, NULL, NULL, NULL); // Blocking until event happens, incoming data in this case.
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (!FD_ISSET(i, &read_set)) continue; // fd is not in tracked set.
            if (i == rc->sd) { // Data available on listening socket.
                rc->data = calloc(UDP_PACKET_SIZE_MAX, 1);

                // Unlike TCP streams, UDP only sends 1 packet at a time, so we read once.
                int bytes_read = read(rc->sd, rc->data, UDP_PACKET_SIZE_MAX);
                rc->data_len = bytes_read;

                // Create arguments for callback function.
                struct cb_args *args = malloc(sizeof(*args)); // create_cb_thread() will free this.
                args->fptr = rc->fptr;
                args->data = rc->data;
                args->data_len = rc->data_len;

                // Create another thread for callback func.
                pthread_t wildthread;
                pthread_create(&wildthread, NULL, create_cb_thread, (void*)args);
                pthread_detach(wildthread);
            } else if (i == rc->pipe[0]) { // Exit loop requested.
                if (is_select_cancelled(rc)) {
                    goto cleanup;
                }
            }
        }
    }
cleanup:
    setsockopt(rc->sd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&rc->group, sizeof(rc->group));
    close(rc->sd);
    close(rc->pipe[0]);
    close(rc->pipe[1]);
    free(rc);
}

static void* create_recv_thread(void* args) {
    _recv((struct mcreceiver*)args);
    return NULL;
}

struct mcreceiver* mc_receiver_init(char *interface, char* multicastip, unsigned short port, void (*callback)(char*, int)) {
    struct mcreceiver* rc = malloc(sizeof(*rc));
    rc->sd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (rc->sd < 0) {
        perror("Opening datagram socket error.");
        goto error;
    }
    if (pipe(rc->pipe) < 0) {
        perror("pipe() syscall error");
        goto error;
    }
    pthread_mutex_init(&rc->mutex, NULL);

    // Enable SO_REUSEADDR to allow multiple instances of this application to receive copies of the multicast datagrams.
    {
        int reuse = 1;
        if (setsockopt(rc->sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
            perror("Setting SO_REUSEADDR error.");
            goto error;
        }
    }

    // Bind to the proper port number with the IP address.
    {
        struct sockaddr_in localsock = {0};
        localsock.sin_family = AF_INET;
        localsock.sin_port = htons(port);
        localsock.sin_addr.s_addr = INADDR_ANY;
        if (bind(rc->sd, (struct sockaddr*)&localsock, sizeof(localsock))) {
            perror("Binding datagram socket error.");
            goto error;
        }
    }

    // Get designated interface's IP address.
    if (interface) {
        struct ifreq ifr;
        ifr.ifr_addr.sa_family = AF_INET;  // ipv4
        strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
        ioctl(rc->sd, SIOCGIFADDR, &ifr);
        char *local_ip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
        if (!local_ip) {
            perror("Failed to get IP address of local interface.");
            goto error;
        }
        rc->group.imr_interface.s_addr = inet_addr(local_ip);
    } else {
        rc->group.imr_interface.s_addr = htonl(INADDR_ANY);
    }

    // Join the multicast group on the designated network interface.
    rc->group.imr_multiaddr.s_addr = inet_addr(multicastip);
    if (setsockopt(rc->sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&rc->group, sizeof(rc->group)) < 0) {
        perror("Adding multicast group error.");
        goto error;
    }

    rc->fptr = callback;
    rc->running = 1;
    pthread_t th;
    pthread_create(&th, NULL, create_recv_thread, (void*)rc);
    pthread_detach(th);

    return rc;

error:
    close(rc->sd);
    free(rc);
    return NULL;
}

void mc_receiver_uinit(struct mcreceiver* rc) {
    set_running(rc, 0);
    cancel_select(rc);
}
