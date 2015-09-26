
#include "ipc.h"

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#define PRINT_SEM_VALUES 	printf("SEM_CLIENT: %d\nSEM_SERVER: %d\n", get_sem_val(session->semid, SEM_CLIENT), get_sem_val(session->semid, SEM_SERVER))
#define PRINT_QSEM_VALUES 	printf("SEM_QUEUE: %d\nSEM_SRV_QUEUE: %d\n", get_sem_val(session->queueid, SEM_QUEUE), get_sem_val(session->queueid, SEM_SRV_QUEUE))

#define SEM_QUEUE			0
#define SEM_SRV_QUEUE		1

#define SEM_SERVER 			0
#define SEM_CLIENT 			1

// #define WAIT_FOR(sem) 		sem_down(session->semid, sem)
// #define UNBLOCK(sem)		sem_up(session->semid, sem)

// #define WAIT_FOR_QUEUE(sem) sem_down(session->queueid, sem)
// #define UNBLOCK_QUEUE(sem) 	sem_up(session->queueid, sem)


void sem_init_with_key(int* semid, key_t semkey, int n);

void sem_init(int* semid, int n);

void sem_queue_init(int* queueid, int n);

void sem_set(int semid, int semnum, int val);

void sem_destroy(int semid);

void sem_queue_destroy(int queueid);

void sem_down(int semid, int semnum);

void sem_up(int semid, int semnum);

int get_sem_val(int semid, int semnum );


#endif
