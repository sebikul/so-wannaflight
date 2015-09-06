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

#define PRINT_SEM_VALUES 	printf("SEM_CLIENT: %d\nSEM_SERVER: %d\n", get_sem_val(SEM_CLIENT), get_sem_val(SEM_SERVER))

#define SEM_QUEUE	0
#define SEM_SERVER 	1
#define SEM_CLIENT 	2

#define WAIT_FOR(sem) 	sem_down(sem)
#define UNBLOCK(sem)	sem_up(sem)

// TODO
typedef struct {

	int semid;
	int semcount;

	int memid;
	char* alloc;

} SHMEM_ALLOC;

static SHMEM_ALLOC shmem;

static int cli_count = 0;


static void shmem_init_with_key(key_t shmemkey){

	if ( (shmem.memid = shmget(shmemkey, SHMEM_SIZE, IPC_CREAT|0666)) == -1 ){
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
	Crea la zona de memoria compartida
 */
static void shmem_init(){

	key_t shmemkey=ftok("../database.sqlite",cli_count);

	shmem_init_with_key(shmemkey);

}

/*
	Inicializa _n_ semaforos
 */
static void sem_init(int n){

	//El id solo usa los 8 bits menos significativos. Seteamos a FF que es la cant maxima de clientes -1 que puede aguantar el servidor
	key_t semkey=ftok("../database.sqlite", 0xFF); 
	
	if ( (shmem.semid = semget(semkey, n, 0)) >= 0 ){
		return;
	}
		
	if ( (shmem.semid = semget(semkey, n, IPC_CREAT|0666)) == -1 ){
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


}

static void sem_destroy(){


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

int ipc_send(DB_DATAGRAM* data){

	if(data->size > SHMEM_SIZE){
		return -1;
	}

	memcpy(shmem.alloc, data, data->size);

	UNBLOCK(SEM_CLIENT); //Desbloqueamos el servidor

	return 0;
	
}

DB_DATAGRAM* ipc_receive(){

	DB_DATAGRAM *datagram, *new_datagram;

	datagram = (DB_DATAGRAM*)shmem.alloc;
	new_datagram = (DB_DATAGRAM*)malloc(datagram->size);

	memcpy(new_datagram,datagram,datagram->size);

	UNBLOCK(SEM_CLIENT);

	return new_datagram;

}

static int get_sem_val(int semnum ){
        return(semctl(shmem.semid, semnum, GETVAL));
}

void ipc_listen(){

	int pid;

	//TODO: Obtener un key unico

	printf("Inicializando IPC de shared memory...\n");

	shmem_init();

	sem_init(3);

	sem_set(SEM_QUEUE, 1);
	sem_set(SEM_CLIENT, 0);
	sem_set(SEM_SERVER, 0);

	printf("Escuchando clientes...\n");

	//devuelta:

	//STEP-0: Bloquea hasta que un cliente ejecute semup
	WAIT_FOR(SEM_CLIENT); 

	printf("Cliente conectado...\n");

	switch ( pid = fork() ){
		case -1:
			printf("fork failed.\n");
			exit(1);
			break;

		case 0: /* hijo */

			//STEP-1			
			WAIT_FOR(SEM_CLIENT); //Esperamos al primer mensaje

			DB_DATAGRAM *datagram=(DB_DATAGRAM*)shmem.alloc;

			DUMP_DATAGRAM(datagram);

			if(datagram->dg_shmemkey==-1){
				printf("Pedido de zona de memoria recibido.\n");
			}else{
				printf("Error en el protocolo de sincronizacion.\n");
				exit(1);
			}

			//shmem.alloc tiene el pedido de una zona de memoria. 

			datagram->dg_shmemkey = ftok("../database.sqlite", cli_count); 

			key_t shmemkey = datagram->dg_shmemkey;

			shmem_destroy();

			shmem_init_with_key(shmemkey);

			//STEP-2
			UNBLOCK(SEM_SERVER); //Despertamos el cliente para que haga attach de la zona de memoria pedida

			printf("Esperando a cliente...\n");

			//STEP-3
			WAIT_FOR(SEM_CLIENT); //Esperamos que haga attach, y continuamos para esperar comandos del cliente

			printf("Sincronizacion completa. Esperando comandos...\n");

			int n = 0;
			
			while (1){

				//STEP-4
				WAIT_FOR(SEM_CLIENT); //Esperamos que mande un comando

				DB_DATAGRAM* dg=(DB_DATAGRAM*)shmem.alloc;

				DUMP_DATAGRAM(dg)

				UNBLOCK(SEM_SERVER); //Desbloqueamos el cliente
				
				n++;
			}

			printf("Servidor hijo termina\n");
			exit(0);
			break;
		
		default: /* padre */
			
			cli_count++;

			sleep(100000);

			//TODO Incrementar el contador para generar claves unicas, GOTO devuelta.

			break;
	}

	//Hacer el fork
	//Hijo inicia comunicacion
	//Padre vuelve a ejecutar listen con el key incrementado

}

void ipc_connect(){

	char buffer[64];
	int n;

	DB_DATAGRAM *datagram;

	printf("Conectando con el servidor...\n");

	shmem_init();

	sem_init(3);

	printf("Esperando en la cola...\n");

	WAIT_FOR(SEM_QUEUE); //Si entra otro cliente, queda bloqueado en una "cola"

	printf("Conectado con el servidor...\n");

	// STEP-0: Desbloqueamos el servidor
	UNBLOCK(SEM_CLIENT);

	datagram = (DB_DATAGRAM*) calloc(1, sizeof(DB_DATAGRAM));

	datagram->size = sizeof(DB_DATAGRAM);
	datagram->dg_shmemkey = -1;
	datagram->opcode=OP_CMD;

	//STEP-1: UP -> Client
	ipc_send(datagram);
	free(datagram);

	printf("Pedido de zona de memoria enviado.\n");

	//STEP-2
	WAIT_FOR(SEM_SERVER);//Esperamos que el servidor llene la shmemkey
	
	printf("Recibiendo zona de memoria...\n");

	//STEP-3 Up -> Client
	datagram = ipc_receive();

	shmem_destroy();

	shmem_init_with_key(datagram->dg_shmemkey);

	free(datagram);
	
	while ( (n = read(0, buffer, SHMEM_SIZE)) > 0 ){

		int size = sizeof(DB_DATAGRAM) + n;

		datagram = (DB_DATAGRAM*) calloc(1, size);

		datagram->size = size;
		datagram->opcode=OP_CMD;
		strcpy(&datagram->dg_cmd, buffer);
		(&datagram->dg_cmd)[n] = 0;

		printf("Enviando comando: %s", &datagram->dg_cmd);

		DUMP_DATAGRAM(datagram);

		//STEP-4 Up -> Client
		ipc_send(datagram);

		//UNBLOCK(SEM_CLIENT); //Desbloqueamos el servidor
		WAIT_FOR(SEM_SERVER);  // Esperamos un mensaje

		free(datagram);
	}

	//Aca el servidor ya escribio la clave de la zona de memoria que hay que mapear

	free(datagram);

	UNBLOCK(SEM_QUEUE); //Cuando terminamos de crear la conexion, se libera el servidor

	shmem_detach();

}



void ipc_disconnect(){

	shmem_destroy();
	sem_destroy();

}