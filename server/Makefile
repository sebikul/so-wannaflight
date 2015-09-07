
.PHONY: all clean sqlite3

all: server.o

run: clean server.o
	./server

sqlite3:
	cd sqlite3; make all

server.o: sqlite3
	gcc -Wall -g -o server -I../include server.c ../ipc/shmem.c sqlite3/sqlite3.o

clean: 
	cd sqlite3; make clean
	rm -f *.o
	rm -f server
