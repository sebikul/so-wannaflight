
GCCFLAGS=-Wall -g -DCLIENT -I../include client.c


.PHONY: all clean shmem socket

all: shmem socket

shmem:
	gcc $(GCCFLAGS) -o client-shmem ../ipc/shmem.c

socket:
	gcc $(GCCFLAGS) -o client-socket ../ipc/socket.c

clean: 
	rm -f *.o
	rm -f client-*
