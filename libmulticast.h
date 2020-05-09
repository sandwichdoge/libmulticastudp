#ifndef INCLUDE_LIBMULTICAST_H
#define INCLUDE_LIBMULTICAST_H
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <netinet/in.h>

struct mcpacket {
    char *data;
    int len;
    char *src_addr;
};

struct mcsender {
    int sd; // socket descriptor
    struct sockaddr_in group; // multicast group
};

struct mcreceiver {
    int sd; // socket descriptor
    int pipe[2]; // used to cancel blocking select()
    int running;
    void (*fptr)(struct mcpacket*); // callback func when read() complete. 1st arg points to received data, 2nd arg is len of data.
    struct ip_mreq group; // multicast group
    pthread_mutex_t mutex;
};

// Init receiver server, bind a callback function to an interface and multicast ip group.
// interface is the name of local interface (e.g. eth0), NULL to bind all interfaces.
// multicastip is something like "226.1.1.1".
struct mcreceiver* mc_receiver_init(char *interface, char* multicastip, unsigned short port, void (*callback)(struct mcpacket*));
// Uninit receiver server.
void mc_receiver_uinit(struct mcreceiver*);

// Init sender server with a multicast ip group.
// interface is the name of local interface (e.g. eth0), NULL to auto select interface.
struct mcsender* mc_sender_init(char *interface, char* multicastip, unsigned short port);
// Uninit sender server.
void mc_sender_uinit(struct mcsender* sc);
// Broadcast message to registered group.
int mc_sender_send(struct mcsender* sc, const char* data, size_t len);

#ifdef __cplusplus
}
#endif
#endif
