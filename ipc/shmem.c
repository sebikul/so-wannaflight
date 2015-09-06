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

#define PRINT_SEM_VALUES 	printf("SEM_CLIENT: %d\nSEM_SERVER: %d\n", get_sem_val(shmem.semid, SEM_CLIENT), get_sem_val(shmem.semid, SEM_SERVER))

#define SEM_QUEUE	0
#define SEM_SERVER 	0
#define SEM_CLIENT 	1

#define WAIT_FOR(sem) 	sem_down(shmem.semid, sem)
#define UNBLOCK(sem)	sem_up(shmem.semid, sem)

#define WAIT_FOR_QUEUE() sem_down(shmem.queueid, SEM_QUEUE)
#define UNBLOCK_QUEUE() sem_up(shmem.queueid, SEM_QUEUE)

#define SERVPRINTE(msg) 		printf("[SERVER #%d] " msg, cli_count)
#define SERVPRINT(msg, ...) 	printf("[SERVER #%d] " msg, cli_count, __VA_ARGS__)

// TODO
typedef struct {

	int semid;
	int queueid;

	int memid;
	char* alloc;

} SHMEM_ALLOC;

static SHMEM_ALLOC shmem;

#define DUMP_SHMEM_DATA() 	printf("\n[\nsemid: %d\nqueueid: %d\nmemid: %d\n]\n\n", shmem.semid, shmem.queueid, shmem.memid);

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

static void sem_init_with_key(key_t semkey){

	//Creamos los semaforos para sincronizar cliente servidor
	if ((shmem.semid = semget(semkey, 2, 0)) >= 0 ){
		printf("Sync semaphores already exist with id: %d\n", shmem.semid);
		return;
	}
		
	if ((shmem.semid = semget(semkey, 2, IPC_CREAT|0666)) == -1 ){
		printf("shmget failed.\n");
		exit(1);
	}

	printf("Initialized sync semaphores with id %d\n", shmem.semid);

}

/*
	Inicializa 2 semaforos para sincronizar cliente-servidor
 */
static void sem_init(){

	key_t semkey=ftok("../database.sqlite", cli_count); 
	
	sem_init_with_key(semkey);

}

static void sem_queue_init(){

	//El id solo usa los 8 bits menos significativos. Seteamos a FF que es la cant maxima de clientes -1 que puede aguantar el servidor
	key_t queuekey=ftok("../database.sqlite", 0xFF); 

	//Creamos el semaforo para la cola de clientes
	if ((shmem.queueid = semget(queuekey, 1, 0)) >= 0 ){
		printf("Queue semaphore already exist with id: %d\n", shmem.queueid);
		return;
	}
		
	if ((shmem.queueid = semget(queuekey, 1, IPC_CREAT|0666)) == -1 ){
		printf("shmget queue failed.\n");
		exit(1);
	}

	printf("Initialized queue semaphore with id %d\n", shmem.queueid);


}

/*
	Setea el valor del semaforo _sem_ a _val_
 */
static void sem_set(int semid, int semnum, int val){

	semctl(semid, semnum, SETVAL, val);

}

static void sem_reset(){

	sem_set(shmem.semid, SEM_CLIENT, 0);
	sem_set(shmem.semid, SEM_SERVER, 0);

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
	semctl(shmem.queueid, 0, IPC_RMID);

}

/*
	Ejecuta down sobre el semaforo
 */
static void sem_down(int semid, int semnum){

	struct sembuf sb;
	
	sb.sem_num = semnum;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;
	semop(semid, &sb, 1);
}

/*
	Ejecuta un up sobre el semaforo
 */
static void sem_up(int semid, int semnum){

	struct sembuf sb;
	
	sb.sem_num = semnum;
	sb.sem_op = 1;
	sb.sem_flg = SEM_UNDO;
	semop(semid, &sb, 1);
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

static int get_sem_val(int semid, int semnum ){
        return(semctl(semid, semnum, GETVAL));
}

static void shmem_serve(){

	while (1){

		//STEP-4
		WAIT_FOR(SEM_CLIENT); //Esperamos que mande un comando

		DB_DATAGRAM* dg=(DB_DATAGRAM*)shmem.alloc;

		SERVPRINT("Mensaje recibido: %s", &dg->dg_cmd);

		//DUMP_DATAGRAM(dg)

		UNBLOCK(SEM_SERVER); //Desbloqueamos el cliente
		
	}

}

void ipc_listen(){

	printf("Inicializando IPC de shared memory...\n");

	shmem_init();

	sem_init();
	sem_queue_init();

	//Inicializamos el valor del semaforo de la cola.
	sem_set(shmem.queueid, SEM_QUEUE, 1);

	printf("Escuchando clientes...\n");	

}

void ipc_accept(){

	int pid;

	//Reseteamos los valores de los semaforos para aceptar al nuevo cliente.
	sem_reset();

	//STEP-0: Bloquea hasta que un cliente ejecute semup
	WAIT_FOR(SEM_CLIENT); 

	printf("Cliente conectado...\n");

	switch ( pid = fork() ){
		case -1:
			printf("fork failed.\n");
			exit(1);
			break;

		case 0: /* hijo */

			SERVPRINTE("Fork creado, esperando pedido de memoria.\n");

			//STEP-1			
			WAIT_FOR(SEM_CLIENT); //Esperamos al primer mensaje

			DB_DATAGRAM *datagram=(DB_DATAGRAM*)shmem.alloc;

			if(datagram->dg_shmemkey==-1){
				SERVPRINTE("Pedido de zona de memoria recibido.\n");
			}else{
				SERVPRINTE("Error en el protocolo de sincronizacion.\n");
				exit(1);
			}

			//shmem.alloc tiene el pedido de una zona de memoria. 

			DUMP_SHMEM_DATA();
			PRINT_SEM_VALUES;

			datagram->dg_shmemkey = ftok("../database.sqlite", cli_count+1); 

			int oldsemid = shmem.semid;

			key_t shmemkey = datagram->dg_shmemkey;

			shmem_init_with_key(shmemkey);
			sem_init_with_key(shmemkey);

			//Reseteamos los nuevos semaforos 
			sem_reset();

			DUMP_SHMEM_DATA()
			PRINT_SEM_VALUES;

			//STEP-2
			//UNBLOCK(SEM_SERVER); //Despertamos el cliente para que haga attach de la zona de memoria pedida
			sem_up(oldsemid,SEM_SERVER);


			//STEP-3
			//WAIT_FOR(SEM_CLIENT); //Esperamos que haga attach, y continuamos para esperar comandos del cliente
			sem_down(oldsemid,SEM_CLIENT);


			SERVPRINTE("Sincronizacion completa. Esperando comandos...\n");

			shmem_serve();

			printf("Servidor hijo termina\n");
			exit(0);
			break;
		
		default: /* padre */
			
			cli_count++;


			//ACA ROMPE
			sleep(1);

			//TODO Incrementar el contador para generar claves unicas.

			break;
	}

}

void ipc_connect(){

	char buffer[64];
	int n;

	DB_DATAGRAM *datagram;

	printf("Conectando con el servidor...\n");

	shmem_init();

	sem_init();
	sem_queue_init();

	printf("Esperando en la cola...\n");

	WAIT_FOR_QUEUE(); //Si entra otro cliente, queda bloqueado en una "cola"

	printf("Conectado con el servidor...\n");

	// STEP-0: Desbloqueamos el servidor
	UNBLOCK(SEM_CLIENT);

	datagram = (DB_DATAGRAM*) calloc(1, sizeof(DB_DATAGRAM));

	datagram->size = sizeof(DB_DATAGRAM);
	datagram->dg_shmemkey = -1;
	datagram->opcode = OP_CMD;

	//STEP-1: UP -> Client
	ipc_send(datagram);
	free(datagram);

	printf("Pedido de zona de memoria enviado.\n");

	//STEP-2
	WAIT_FOR(SEM_SERVER);//Esperamos que el servidor llene la shmemkey
	
	printf("Recibiendo zona de memoria...\n");

	//STEP-3 Up -> Client
	datagram = ipc_receive();

	DUMP_DATAGRAM(datagram);

	shmem_destroy();

	shmem_init_with_key(datagram->dg_shmemkey);
	sem_init_with_key(datagram->dg_shmemkey);

	free(datagram);

	UNBLOCK_QUEUE(); //Cuando terminamos de crear la conexion, se libera el servidor
	
	printf("Listo para enviar comandos...\n");

	while ( (n = read(0, buffer, SHMEM_SIZE)) > 0 ){

		int size = sizeof(DB_DATAGRAM) + n;

		datagram = (DB_DATAGRAM*) calloc(1, size);

		datagram->size = size;
		datagram->opcode=OP_CMD;
		strcpy(&datagram->dg_cmd, buffer);
		(&datagram->dg_cmd)[n] = 0;

		printf("Enviando comando: %s", &datagram->dg_cmd);

		//DUMP_DATAGRAM(datagram);

		//STEP-4 Up -> Client
		ipc_send(datagram);

		//UNBLOCK(SEM_CLIENT); //Desbloqueamos el servidor
		WAIT_FOR(SEM_SERVER);  // Esperamos un mensaje

		free(datagram);
	}

	
	shmem_detach();

}



void ipc_disconnect(){

	shmem_destroy();
	sem_destroy();

}