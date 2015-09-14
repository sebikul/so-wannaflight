

all: clean server client

server:
	cd server; make all

client:
	cd client; make all

clean:
	cd server; make clean
	cd client; make clean

shmem:
	cd server; make shmem
	cd client; make shmem

socket:
	cd server; make socket
	cd client; make socket
	
.PHONY: all clean server client