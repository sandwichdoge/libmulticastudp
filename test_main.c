#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "libmulticast.h"

#define MCAST_IP "226.1.1.1"
#define PORT 4321
#define DATA "Hello THERE"

struct mcreceiver *rc = NULL;
struct mcsender *sc = NULL;

void sig_handler(int signo)
{
    if (signo == SIGINT) {
        printf("received SIGINT\n");
        if (rc) mc_receiver_uinit(rc);
        exit(0);
    }
}

void cb(char *data, int len) {
    printf("[%s]-[Len:%d]\n", data, len);
}

int main() {
    signal(SIGINT, sig_handler);
    sc = mc_sender_init(NULL, MCAST_IP, PORT);
    rc = mc_receiver_init(NULL, MCAST_IP, PORT, &cb);
    printf("Registered to %s:%d. Handler:[%p]\n", MCAST_IP, PORT, (void*)rc);
    printf("Listening..\n");
    while (1) {
        sleep(2);
        printf("Sending %s. Loopback disabled so we won't receive our own datagrams. Need another device to test.\n", DATA);
        mc_sender_send(sc, DATA, strlen(DATA) + 1);
    }
    return 0;
}
