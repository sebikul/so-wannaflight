#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "filemutex.h"

struct fmutex_struct {
	char* path;
	int fd;
};

fmutex_t fmutex_new(const char* path) {

	fmutex_t mutex = (fmutex_t) malloc(sizeof(fmutex_t));
	
	mutex->path = malloc(strlen(path) + 1);

	mutex->fd = -2;

	strcpy(mutex->path, path);

	return mutex;

}

void fmutex_enter(fmutex_t mutex) {

	struct flock lock;
	int code;

	mutex->fd = open(mutex->path, O_CREAT | O_WRONLY);

	printf("Opening %s, fd: %d, errno: %d\n", mutex->path, mutex->fd, errno);

	memset(&lock, 0, sizeof(struct flock));
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;

	printf("Locking file: %s\n", mutex->path);

	code = fcntl(mutex->fd, F_SETLKW, &lock);
}

void fmutex_leave(fmutex_t mutex) {

	struct flock lock;

	if (mutex->fd == -2) {
		printf("Mutex no inicializado!\n");
	}

	memset(&lock, 0, sizeof(struct flock));
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;

	printf("Unlocking file: %s\n", mutex->path);
	fcntl(mutex->fd, F_SETLK, &lock);
	close(mutex->fd);

}

void fmutex_free(fmutex_t mutex) {

	free(mutex->path);
	free(mutex);

}
