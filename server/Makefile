GCCFLAGS=-Wall -ggdb -DSERVER -pthread -ldl -I../include sqlite3/sqlite3.o server.c db/database.c

.PHONY: all clean sqlite3 socket shmem fifo

all: shmem socket fifo

run: clean server.o
	./server

sqlite3:
	cd sqlite3; make all

shmem: sqlite3
	gcc $(GCCFLAGS) -DSHMEM -o server-shmem ../ipc/shmem.c ../ipc/lib/semaphore.c

socket: sqlite3
	gcc $(GCCFLAGS) -DSOCKET -o server-socket ../ipc/socket.c

fifo: sqlite3
	gcc $(GCCFLAGS) -DFIFO -o server-fifo ../ipc/fifo.c  ../ipc/lib/semaphore.c

clean: 
	cd sqlite3; make clean
	rm -f *.o
	rm -rf server-*
