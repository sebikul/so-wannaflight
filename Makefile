

all: server

server:
	cd server; make all

clean:
	cd server; make clean
	
.PHONY: all clean