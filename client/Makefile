GCCFLAGS=-Wall -g -DCLIENT -I../include client.c

.PHONY: all clean shmem socket fifo

all: shmem socket fifo

shmem:
	gcc $(GCCFLAGS) -o client-shmem ../ipc/shmem.c

socket:
	gcc $(GCCFLAGS) -o client-socket ../ipc/socket.c

fifo:
	gcc $(GCCFLAGS) -o client-fifo ../ipc/fifo.c

clean: 
	rm -f *.o
	rm -rf client-*
