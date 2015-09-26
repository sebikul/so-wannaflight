#include <sys/sem.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <stdio.h>

#include "semaphore.h"

void sem_init_with_key(int* semid, key_t semkey, int n) {

	//Creamos los semaforos para sincronizar cliente servidor
	if ((*semid = semget(semkey, n, 0)) >= 0 ) {
		printf("Sync semaphores already exist with id: %d\n", *semid);
		return;
	}

	if ((*semid = semget(semkey, n, IPC_CREAT | 0666)) == -1 ) {
		printf("semget failed.\n");
		exit(1);
	}

	printf("Initialized sync semaphores with id %d\n", *semid);

}


void sem_init(int* semid, int n) {

	key_t semkey = ftok("../database.sqlite", 0);

	sem_init_with_key(semid, semkey, n);

}

void sem_queue_init(int* queueid, int n) {

	//El id solo usa los 8 bits menos significativos. Seteamos a FF que es la cant maxima de clientes -1 que puede aguantar el servidor
	key_t queuekey = ftok("../database.sqlite", 0xFF);

	//Creamos el semaforo para la cola de clientes
	if ((*queueid = semget(queuekey, n, 0)) >= 0 ) {
		printf("Queue semaphore already exist with id: %d\n", *queueid);
		return;
	}

	if ((*queueid = semget(queuekey, n, IPC_CREAT | 0666)) == -1 ) {
		printf("semget queue failed.\n");
		exit(1);
	}

	printf("Initialized queue semaphores with id %d\n", *queueid);

}

void sem_set(int semid, int semnum, int val) {

	semctl(semid, semnum, SETVAL, val);

}

void sem_destroy(int semid) {

	printf("Destroying sync semaphores with id %d\n", semid);

	//Destruimos los semaforos
	semctl(semid, 0, IPC_RMID);

}

void sem_queue_destroy(int queueid) {

	semctl(queueid, 0, IPC_RMID);

}

/*
	Ejecuta down sobre el semaforo
 */
void sem_down(int semid, int semnum) {

	struct sembuf sb;

	sb.sem_num = semnum;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;
	semop(semid, &sb, 1);
}

/*
	Ejecuta un up sobre el semaforo
 */
void sem_up(int semid, int semnum) {

	struct sembuf sb;

	sb.sem_num = semnum;
	sb.sem_op = 1;
//	sb.sem_flg = SEM_UNDO; FUCK YOU UNDO!
	semop(semid, &sb, 1);
}

int get_sem_val(int semid, int semnum ) {
	return (semctl(semid, semnum, GETVAL));
}
