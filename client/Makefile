GCCFLAGS=-Wall -g -DCLIENT -I../include client.c

.PHONY: all clean shmem socket fifo

all: shmem socket fifo filesig

shmem:
	gcc $(GCCFLAGS) -o client-shmem ../ipc/shmem.c  ../ipc/lib/semaphore.c

socket:
	gcc $(GCCFLAGS) -o client-socket ../ipc/socket.c

fifo:
	gcc $(GCCFLAGS) -o client-fifo ../ipc/fifo.c  ../ipc/lib/semaphore.c

filesig:
	gcc $(GCCFLAGS) -o client-filesig ../ipc/filesig.c

clean: 
	rm -f *.o
	rm -rf client-*
