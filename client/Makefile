
.PHONY: all clean

all: client.o

run: clean client.o
	./client

client.o:
	gcc -Wall -g -o client -I../include client.c ../ipc/shmem.c
clean: 
	rm -f *.o
	rm -f client
