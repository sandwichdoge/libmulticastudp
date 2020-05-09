### libmulticastudp

For sending and receiving multicast UDP packets within a network, based on Berkeley socket.

To test accurately, run python test scripts in 'multicasting_test_scripts'
on another computer in the same network.

- Kernel configs:
```
CONFIG_IP_MULTICAST=1
```

#### Build
```
make
make test
```

#### Usage

Register to receive multicast udp packets from group "226.1.1.1", port 4321:
```
static int running = 1;

void cb(struct mcpacket* packet) {
    printf("[%s]-[Len:%d]-[From:%s]\n", packet->data, packet->len, packet->src_addr);
    running = 0; // Exit after receiving 1st packet.
}

int main() {
    struct mcreceiver *rc = mc_receiver_init(NULL, "226.1.1.1", 4321, &cb);
    while (running) {
        sleep(1);
    }
    mc_receiver_uinit(rc);
    return 0;
}
```

Send multicast packets:
```
int main() {
    struct mcsender *sc = mc_sender_init(NULL, "226.1.1.1", 4321);
    mc_sender_send(sc, "Hello\0", strlen("Hello") + 1);
    mc_sender_uinit(sc);
    return 0;
}
```

#### Linking
```
#include "libmulticast.h"
```

Require pthread.
```
$(CC) main.c -lmulticast -pthread
```
