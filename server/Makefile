GCCFLAGS=-Wall -ggdb -DSERVER -I../include sqlite3/sqlite3.o server.c 

.PHONY: all clean sqlite3 socket shmem

all: shmem socket

run: clean server.o
	./server

sqlite3:
	cd sqlite3; make all

shmem: sqlite3
	gcc $(GCCFLAGS) -o server-shmem ../ipc/shmem.c 


socket: sqlite3
	gcc $(GCCFLAGS) -o server-socket ../ipc/socket.c

clean: 
	cd sqlite3; make clean
	rm -f *.o
	rm -rf server-*
