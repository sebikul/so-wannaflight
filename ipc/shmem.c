#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>


#include "config.h"
#include "database.h"
#include "ipc.h"

#define SEM_QUEUE				0
#define SEM_WAIT_FOR_SERVER 	1
#define SEM_WAIT_FOR_CLIENT 	2

// TODO
typedef struct {

	int semid;
	int semcount;

	int memid;
	char* alloc;

} SHMEM_ALLOC;

static SHMEM_ALLOC shmem;


/*
	Crea la zona de memoria compartida
 */
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

/*
	Inicializa _n_ semaforos
 */
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

/*
	Setea el valor del semaforo _sem_ a _val_
 */
static void sem_set(int semnum, int val){

	semctl(shmem.semid, semnum, SETVAL, val);

}

static void shmem_detach(){
	shmdt(shmem.alloc);
}

/*
	Elimina la zona de memoria compartida.
 */
static void shmem_destroy(){

	shmem_detach();

	//Marcamos la memoria como eliminable
	shmctl(shmem.memid, IPC_RMID, 0);

	//Destruimos los semaforos
	semctl(shmem.semid, 0, IPC_RMID);

}

/*
	Ejecuta down sobre el semaforo
 */
static void sem_down(int semnum){

	struct sembuf sb;
	
	sb.sem_num = semnum;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;
	semop(shmem.semid, &sb, 1);
}

/*
	Ejecuta un up sobre el semaforo
 */
static void sem_up(int semnum){

	struct sembuf sb;
	
	sb.sem_num = semnum;
	sb.sem_op = 1;
	sb.sem_flg = SEM_UNDO;
	semop(shmem.semid, &sb, 1);
}

void ipc_listen(){

	int pid,i=0;

	//TODO: Obtener un key unico

	printf("Inicializando IPC de shared memory...\n");

	shmem_init();

	sem_init(3);

	sem_set(SEM_QUEUE, 1);
	sem_set(SEM_WAIT_FOR_CLIENT, 0);
	sem_set(SEM_WAIT_FOR_SERVER, 0);

	printf("Escuchando clientes...\n");

	//devuelta:

	sem_down(SEM_WAIT_FOR_CLIENT); //Bloquea hasta que un cliente ejecute semup

	printf("Cliente conectado...\n");

	switch ( pid = fork() ){
		case -1:
			printf("fork failed.\n");
			exit(1);
			break;

		case 0: /* hijo */
			
			sem_down(SEM_WAIT_FOR_CLIENT); //Esperamos al primer mensaje

			while (1){

				printf("Esperando a cliente...\n");
				// sem_up(SEM_WAIT_FOR_CLIENT); 
				

				printf("Servidor hijo lee: %s", shmem.alloc);
				if ( memcmp(shmem.alloc, "end", 3) == 0 ){
					break;
				}

				sem_up(SEM_WAIT_FOR_SERVER); //Desbloqueamos el cliente
				sem_down(SEM_WAIT_FOR_CLIENT); //Esperamos que mande un comando

				i++;
			}

			printf("Servidor hijo termina\n");
			break;
		
		default: /* padre */
			
			sleep(100000);

			//TODO Incrementar el contador para generar claves unicas, GOTO devuelta.

			break;
	}

	//Hacer el fork
	//Hijo inicia comunicacion
	//Padre vuelve a ejecutar listen con el key incrementado

	shmem_destroy();

}

void ipc_connect(){

	int n;

	printf("Connecting to server...\n");

	shmem_init();

	sem_init(3);

	printf("Esperando en la cola...\n");

	sem_down(SEM_QUEUE); //Si entra otro cliente, queda bloqueado en una "cola"

	printf("Conectado con el servidor...\n");

	sem_up(SEM_WAIT_FOR_CLIENT);//Desbloqueamos el servidor

	//sem_down(SEM_WAIT_FOR_SERVER); //Quedamos a la espera de mesajes

	while ( (n = read(0, shmem.alloc, SHMEM_SIZE)) > 0 ){

				//sem_down(SEM_WAIT_FOR_SERVER);

				printf("Cliente escribe: %.*s", n, shmem.alloc);

				sem_up(SEM_WAIT_FOR_CLIENT); //Desbloqueamos el servidor
				sem_down(SEM_WAIT_FOR_SERVER);  // Esperamos un mensaje
	}

	//Aca el servidor ya escribio la clave de la zona de memoria que hay que mapear

	sem_up(SEM_QUEUE); //Cuando terminamos de crear la conexion, se libera el servidor

	shmem_detach();

}