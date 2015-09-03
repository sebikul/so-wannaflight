#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>


#include "config.h"
#include "database.h"
#include "ipc.h"

// TODO
typedef struct {

	int semid;
	int semcount;

	int memid;
	char* alloc;

} SHMEM_ALLOC;

static SHMEM_ALLOC shmem;


static void shmem_init(){

	if ( (shmem.memid = shmget(SHMEM_KEY, SHMEM_SIZE, IPC_CREAT|0666)) == -1 ){
		printf("shmget failed.\n");
		exit(1);
	}

	if ( !(shmem.alloc = shmat(shmem.memid, NULL, 0)) ){
		printf("shmget failed.\n");
		exit(1);
	}

	printf("Initialized shared memory zone with id %d\n", shmem.memid);

}

static void sem_init(int n){

	if ( (shmem.semid = semget(SEM_KEY, n, 0)) >= 0 ){
		return;
	}
		
	if ( (shmem.semid = semget(SEM_KEY, n, IPC_CREAT|0666)) == -1 ){
		printf("shmget failed.\n");
		exit(1);
	}

	shmem.semcount=n;


	printf("Initialized %d semaphores with id %d\n", n, shmem.semid);

}

static void sem_set(int val){

	//TODO hacer un for por si n>1
	semctl(shmem.semid, 0, SETVAL, val);

}

static void shmem_destroy(){

	shmdt(shmem.alloc);
	shmctl(shmem.memid, IPC_RMID, 0);

	semctl(shmem.semid, 0, IPC_RMID);

}

// static void sem_destroy(){

// }

static void sem_down(void){

	struct sembuf sb;
	
	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;
	semop(shmem.semid, &sb, 1);
}

static void sem_up(void){

	struct sembuf sb;
	
	sb.sem_num = 0;
	sb.sem_op = 1;
	sb.sem_flg = SEM_UNDO;
	semop(shmem.semid, &sb, 1);
}

void ipc_listen(){

	printf("Inicializando IPC de shared memory...\n");

	shmem_init();

	sem_init(1);

	sem_set(0);

	printf("Escuchando clientes\n");

	sem_down(); //Bloquea hasta que un cliente ejecute semup

	shmem_destroy();

}

void ipc_connect(){

	printf("Connecting to server...\n");

	sem_init(1);

	sem_up();

}