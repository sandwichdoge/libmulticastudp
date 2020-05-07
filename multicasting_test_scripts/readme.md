python multicast
================

This repository contains a simple multicast client/server implementation.

The client and the server must use the same multicast address and port.

The official list from IANA may be found [here](http://www.iana.org/assignments/multicast-addresses/multicast-addresses.xhtml)


receiver.py
-------------

The server that waits for the multicast data. Could be run on multiple machines.

Note that the receiver must be on a different machine
see:
[http://stackoverflow.com/questions/1719156/is-there-a-way-to-test-multicast-ip-on-same-box]


sender.py
----------

The client that sends the multicast trafic.

It just sends the data, no check is perfomed to know whether or not anyone is listening or receiving.
