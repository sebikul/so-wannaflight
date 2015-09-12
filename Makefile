

all: clean server client

server:
	cd server; make all

client:
	cd client; make all

clean:
	cd server; make clean
	cd client; make clean
	
.PHONY: all clean server client