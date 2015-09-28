GCCFLAGS=-Wall -Wextra -pedantic -Wno-unused-parameter -g -DSERVER -pthread -ldl -I../include sqlite3/sqlite3.o server.c db/database.c libserver.c
GCC=gcc-5

.PHONY: all clean sqlite3 socket shmem fifo files msgqueue

all: shmem socket fifo files msgqueue

run: clean server.o
	./server

sqlite3:
	cd sqlite3; make all

shmem: sqlite3
	$(GCC) $(GCCFLAGS) -DSHMEM -o server-shmem ../ipc/shmem.c ../ipc/lib/semaphore.c

socket: sqlite3
	$(GCC) $(GCCFLAGS) -DSOCKET -o server-socket ../ipc/socket.c

fifo: sqlite3
	$(GCC) $(GCCFLAGS) -DFIFO -o server-fifo ../ipc/fifo.c  ../ipc/lib/semaphore.c

files: sqlite3
	$(GCC) $(GCCFLAGS) -DFILES -o server-files ../ipc/files.c ../ipc/lib/semaphore.c

msgqueue:
	$(GCC) $(GCCFLAGS) -DMSGQUEUE -o server-msgqueue ../ipc/msgqueue.c

clean: 
	cd sqlite3; make clean
	rm -f *.o
	rm -rf server-*
	rm -f files/*
