GCCFLAGS=-Wall -Wextra -pedantic -Wno-unused-parameter -g -DCLIENT -I../include 
SOURCES=client.c libclient.c

.PHONY: all clean shmem socket fifo filesig msgqueue sqlite3 localclient

all: shmem socket fifo filesig msgqueue localclient

shmem:
	gcc $(GCCFLAGS) $(SOURCES) -o client-shmem ../ipc/shmem.c  ../ipc/lib/semaphore.c

socket:
	gcc $(GCCFLAGS) $(SOURCES) -o client-socket ../ipc/socket.c

fifo:
	gcc $(GCCFLAGS) $(SOURCES) -o client-fifo ../ipc/fifo.c  ../ipc/lib/semaphore.c

filesig:
	gcc $(GCCFLAGS) $(SOURCES) -o client-filesig ../ipc/filesig.c

msgqueue:
	gcc $(GCCFLAGS) $(SOURCES) -DMSGQUEUE  -o client-msgqueue ../ipc/msgqueue.c

localclient: sqlite3
	gcc $(GCCFLAGS) -I../server localclient.c libclient.c  ../ipc/lib/filemutex.c ../server/{libserver.c,db/database.c,sqlite3/sqlite3.o}  -o localclient 

sqlite3:
	cd ../server/sqlite3; make all

clean: 
	rm -f *.o
	rm -rf client-*
