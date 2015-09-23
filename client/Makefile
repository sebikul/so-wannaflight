GCCFLAGS=-Wall -g -DCLIENT -I../include client.c

.PHONY: all clean shmem socket fifo filesig msgqueue

all: shmem socket fifo filesig msgqueue

shmem:
	gcc $(GCCFLAGS) -o client-shmem ../ipc/shmem.c  ../ipc/lib/semaphore.c

socket:
	gcc $(GCCFLAGS) -o client-socket ../ipc/socket.c

fifo:
	gcc $(GCCFLAGS) -o client-fifo ../ipc/fifo.c  ../ipc/lib/semaphore.c

filesig:
	gcc $(GCCFLAGS) -o client-filesig ../ipc/filesig.c

msgqueue:
	gcc $(GCCFLAGS) -DMSGQUEUE  -o client-msgqueue ../ipc/msgqueue.c

clean: 
	rm -f *.o
	rm -rf client-*
