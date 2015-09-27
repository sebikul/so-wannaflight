
#ifndef FILEMUTEX_H
#define FILEMUTEX_H

struct fmutex_struct;
typedef struct fmutex_struct* fmutex_t;


fmutex_t fmutex_new(const char* path);

void fmutex_enter(fmutex_t mutex);

void fmutex_leave(fmutex_t mutex);

void fmutex_free(fmutex_t mutex);

#endif
