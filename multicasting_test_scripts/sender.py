#
# mostly copied from
#   http://bioportal.weizmann.ac.il/course/python/PyMOTW/PyMOTW/docs/socket/multicast.html
#

import socket
import struct
import sys
import time

message = 'data worth repeating'
multicast_group = ('226.1.1.1', 4321)

# Create the datagram socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Set a timeout so the socket does not block indefinitely when trying
# to receive data.
sock.settimeout(0.2)

counter = 0

try:
    
    while True:
        counter +=1

        # Send data to the multicast group
        print >>sys.stderr, '%d: sending "%s"' % (counter, message )
        sent = sock.sendto(message, multicast_group)
        
        time.sleep( 5 )
    
finally:
    print >>sys.stderr, 'closing socket'
    sock.close()
