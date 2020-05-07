OBJECTS=libmulticast_recv.o \
		libmulticast_send.o

CFLAGS+=-O2 -Wall -Wpedantic# -g -fsanitize=thread

all: BUILD_LIB

BUILD_LIB: $(OBJECTS)
	$(AR) crf libmulticast.a $(OBJECTS)

test:
	$(CC) $(CFLAGS) test_main.c libmulticast.a -o test.out -pthread

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o *.a *.out
	