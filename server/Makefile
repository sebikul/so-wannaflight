
.PHONY: all clean

all: server.o

run: clean server.o
	./server

server.o:
	gcc -Wall -g -o server -I../include server.c ../ipc/shmem.c
clean: 
	rm -f *.o
	rm -f server
