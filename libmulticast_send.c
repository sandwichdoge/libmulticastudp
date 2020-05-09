#include "libmulticast.h"
#include "libmulticast_defs.h"
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct mcsender* mc_sender_init(char *interface, char* multicastip, unsigned short port) {
    struct mcsender *sc = malloc(sizeof(*sc));
    sc->sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sc->sd < 0) {
        perror("Opening datagram socket error.");
        goto error;
    }

    // Init UDP interface with address and port.
    {
        memset((char *)&sc->group, 0, sizeof(sc->group));
        sc->group.sin_family = AF_INET;
        sc->group.sin_addr.s_addr = inet_addr(multicastip);
        sc->group.sin_port = htons(port);
    }

    // Disable loopback so we don't receive our own datagrams.
    // Set TTL to 1. This is important, if too high means we'll leak packets to ISP's backbone or the Internet.
    {
        unsigned char loopch = 0;
        setsockopt(sc->sd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopch, sizeof(loopch));
        unsigned char ttl = LIBMULTICAST_TTL_DEFAULT;
        setsockopt(sc->sd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    }

    if (interface) {
        // Get designated interface's IP address. 
        struct ifreq ifr;
        ifr.ifr_addr.sa_family = AF_INET;  // ipv4
        strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
        ioctl(sc->sd, SIOCGIFADDR, &ifr);
        char *local_ip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

        // Prepare to send outbound packet. IP address must be associated with a multicast capable interface.
        struct in_addr local_interface;
        local_interface.s_addr = inet_addr(local_ip);
        if (setsockopt(sc->sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&local_interface, sizeof(local_interface)) < 0) {
            perror("Setting local interface error.");
            goto error;
        }
    }

    return sc;

error:
    close(sc->sd);
    free(sc);
    return NULL;
}

void mc_sender_uinit(struct mcsender* sc) {
    close(sc->sd);
    free(sc);
}

int mc_sender_send(struct mcsender* sc, const char* data, size_t len) {
    // Make a copy of data to send. Because socket's speed is slow, data's lifetime may run out on caller.
    char *buf = calloc(len, 1);
    memcpy(buf, data, len);

    int rc = sendto(sc->sd, buf, len, 0, (struct sockaddr *)&sc->group, sizeof(sc->group));
    if (rc < 0) perror("Error sending data.\n");

    free(buf);
    return rc;
}