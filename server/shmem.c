
#include "config.h"
#include "database.h"
#include "ipc.h"

char * shmem_init(){

	char* mem;
	int memid;
	
	if ( (memid = shmget(SHMEM_KEY, SHMEM_SIZE, IPC_CREAT|0666)) == -1 ){
		printf("shmget failed.\n");
		exit(1);
	}

	if ( !(mem = shmat(memid, NULL, 0)) ){
		printf("shmget failed.\n");
		exit(1);
	}

	return mem;

}

void initmutex(void){

	if ( (semid = semget(semkey, 1, 0)) >= 0 ){
		return;
	}
		
	if ( (semid = semget(semkey, 1, IPC_CREAT|0666)) == -1 ){
		printf("shmget failed.\n");
		exit(1);
	}
	semctl(semid, 0, SETVAL, 1);
}

void enter(void){

	struct sembuf sb;
	
	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;
	semop(semid, &sb, 1);
}

void leave(void){

	struct sembuf sb;
	
	sb.sem_num = 0;
	sb.sem_op = 1;
	sb.sem_flg = SEM_UNDO;
	semop(semid, &sb, 1);
}

void listen(){


	
	
}