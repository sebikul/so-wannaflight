
.PHONY: all clean sqlite3

all: shmem

run: clean server.o
	./server

sqlite3:
	cd sqlite3; make all


shmem: sqlite3
	gcc -Wall -g -o server-shmem -I../include server.c ../ipc/shmem.c sqlite3/sqlite3.o


socket: sqlite3
	gcc -Wall -g -o server-socket -I../include server.c ../ipc/socket.c sqlite3/sqlite3.o


clean: 
	cd sqlite3; make clean
	rm -f *.o
	rm -f server-*
