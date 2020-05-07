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

void cb(char *data, int len) {
    printf("[%s]-[Len:%d]\n", data, len);
    running = 0;
}

int main() {
    struct mcreceiver *rc = receiver_init(NULL, "226.1.1.1", 4321, &cb);
    while (running) {
        sleep(1);
    }
    receiver_uinit(rc);
    return 0;
}
```

Send multicast packets:
```
int main() {
    struct mcsender *sc = sender_init(NULL, "226.1.1.1", 4321);
    sender_send(sc, "Hello\0", strlen("Hello") + 1);
    return 0;
}
```
