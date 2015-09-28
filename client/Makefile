GCCFLAGS=-Wall -Wextra -pedantic -Wno-unused-parameter -g -DCLIENT -I../include 
SOURCES=client.c libclient.c
GCC=gcc-5

.PHONY: all clean shmem socket fifo files msgqueue sqlite3 localclient

all: shmem socket fifo files msgqueue localclient

shmem:
	$(GCC) $(GCCFLAGS) $(SOURCES) -o client-shmem ../ipc/shmem.c  ../ipc/lib/semaphore.c

socket:
	$(GCC) $(GCCFLAGS) $(SOURCES) -o client-socket ../ipc/socket.c

fifo:
	$(GCC) $(GCCFLAGS) $(SOURCES) -o client-fifo ../ipc/fifo.c  ../ipc/lib/semaphore.c

msgqueue:
	$(GCC) $(GCCFLAGS) $(SOURCES) -DMSGQUEUE  -o client-msgqueue ../ipc/msgqueue.c

localclient: sqlite3
	$(GCC) $(GCCFLAGS) -I../server localclient.c libclient.c  ../ipc/lib/filemutex.c ../server/{libserver.c,db/database.c,sqlite3/sqlite3.o}  -o localclient 

files:
	$(GCC) $(GCCFLAGS) $(SOURCES) -DFILES -o client-files ../ipc/files.c

sqlite3:
	cd ../server/sqlite3; make all

clean: 
	rm -f *.o
	rm -rf client-*
