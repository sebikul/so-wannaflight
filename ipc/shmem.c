#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>

#include "config.h"
#include "database.h"
#include "ipc.h"

#define PRINT_SEM_VALUES 	printf("SEM_CLIENT: %d\nSEM_SERVER: %d\n", get_sem_val(shmem.semid, SEM_CLIENT), get_sem_val(shmem.semid, SEM_SERVER))
#define PRINT_QSEM_VALUES 	printf("SEM_QUEUE: %d\nSEM_SRV_QUEUE: %d\n", get_sem_val(shmem.queueid, SEM_QUEUE), get_sem_val(shmem.queueid, SEM_SRV_QUEUE))


#define SEM_QUEUE			0
#define SEM_SRV_QUEUE		1

#define SEM_SERVER 			0
#define SEM_CLIENT 			1

#define WAIT_FOR(sem) 		sem_down(shmem.semid, sem)
#define UNBLOCK(sem)		sem_up(shmem.semid, sem)

#define WAIT_FOR_QUEUE(sem) sem_down(shmem.queueid, sem)
#define UNBLOCK_QUEUE(sem) 	sem_up(shmem.queueid, sem)


typedef struct {

	int semid;
	int queueid;

	int memid;
	char* alloc;

} SHMEM_ALLOC;

static SHMEM_ALLOC shmem;

#define DUMP_SHMEM_DATA() 	printf("\n[\nsemid: %d\nqueueid: %d\nmemid: %d\nalloc: %p\n]\n\n", shmem.semid, shmem.queueid, shmem.memid,shmem.alloc);

#ifdef SERVER
extern int cli_count;
#endif

static void shmem_init_with_key(key_t shmemkey){

	if ( (shmem.memid = shmget(shmemkey, SHMEM_SIZE, IPC_CREAT|0666)) == -1 ){
		printf("shmget failed.\n");
		exit(1);
	}

	if ( !(shmem.alloc = shmat(shmem.memid, NULL, 0)) ){
		printf("shmget failed.\n");
		exit(1);
	}

	memset(shmem.alloc, 0, SHMEM_SIZE);

	//printf("Initialized shared memory zone with id %d\n", shmem.memid);

}

/*
	Crea la zona de memoria compartida
 */
static void shmem_init(){

	key_t shmemkey = ftok("../database.sqlite", 0);

	shmem_init_with_key(shmemkey);

}

static void sem_init_with_key(key_t semkey){

	//Creamos los semaforos para sincronizar cliente servidor
	if ((shmem.semid = semget(semkey, 2, 0)) >= 0 ){
		//printf("Sync semaphores already exist with id: %d\n", shmem.semid);
		return;
	}
		
	if ((shmem.semid = semget(semkey, 2, IPC_CREAT|0666)) == -1 ){
		printf("shmget failed.\n");
		exit(1);
	}

	//printf("Initialized sync semaphores with id %d\n", shmem.semid);

}

/*
	Inicializa 2 semaforos para sincronizar cliente-servidor
 */
static void sem_init(){

	key_t semkey = ftok("../database.sqlite", 0); 
	
	sem_init_with_key(semkey);

}

static void sem_queue_init(){

	//El id solo usa los 8 bits menos significativos. Seteamos a FF que es la cant maxima de clientes -1 que puede aguantar el servidor
	key_t queuekey=ftok("../database.sqlite", 0xFF); 

	//Creamos el semaforo para la cola de clientes
	if ((shmem.queueid = semget(queuekey, 2, 0)) >= 0 ){
		//printf("Queue semaphore already exist with id: %d\n", shmem.queueid);
		return;
	}
		
	if ((shmem.queueid = semget(queuekey, 2, IPC_CREAT|0666)) == -1 ){
		printf("shmget queue failed.\n");
		exit(1);
	}

	//printf("Initialized queue semaphores with id %d\n", shmem.queueid);


}

/*
	Setea el valor del semaforo _sem_ a _val_
 */
static void sem_set(int semid, int semnum, int val){

	semctl(semid, semnum, SETVAL, val);

}

static void shmem_detach(){

	printf("Detaching shared memory zone...\n");

	shmdt(shmem.alloc);

}

#ifdef SERVER
static void sem_reset(){

	sem_set(shmem.semid, SEM_CLIENT, 0);
	sem_set(shmem.semid, SEM_SERVER, 0);

}

/*
	Elimina la zona de memoria compartida.
 */
static void shmem_destroy(){

	shmem_detach();

	CLIPRINTE("Destroying shared memory zone...\n");

	//Marcamos la memoria como eliminable
	shmctl(shmem.memid, IPC_RMID, 0);

}

static void sem_destroy(){

	CLIPRINTE("Destroying semaphores...\n");

	//Destruimos los semaforos
	semctl(shmem.semid, 0, IPC_RMID);
	
}

static void sem_queue_destroy(){

	semctl(shmem.queueid, 0, IPC_RMID);

}

#endif
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

static int get_sem_val(int semid, int semnum ){
        return(semctl(semid, semnum, GETVAL));
}


#ifdef SERVER
/*
	---------------------------------------------------
	Capa de comunicacion para el SERVIDOR

	---------------------------------------------------
 */
DB_DATAGRAM* ipc_receive(){

	WAIT_FOR(SEM_CLIENT); //Esperamos que mande un comando

	DB_DATAGRAM *datagram, *new_datagram;

	datagram = (DB_DATAGRAM*)shmem.alloc;
	new_datagram = (DB_DATAGRAM*)malloc(datagram->size);

	memcpy(new_datagram, datagram, datagram->size);

	return new_datagram;

}

int ipc_send(DB_DATAGRAM* data){

	if(data->size > SHMEM_SIZE){
		return -1;
	}

	memcpy(shmem.alloc, data, data->size);

	UNBLOCK(SEM_SERVER); //Desbloqueamos el cliente

	return 0;
	
}

#else
/*
	---------------------------------------------------
	Capa de comunicacion para el CLIENTE

	---------------------------------------------------
 */

int ipc_send(DB_DATAGRAM* data){

	if(data->size > SHMEM_SIZE){
		return -1;
	}

	memcpy(shmem.alloc, data, data->size);

	UNBLOCK(SEM_CLIENT); //Desbloqueamos el servidor

	return 0;
	
}

DB_DATAGRAM* ipc_receive(){

	WAIT_FOR(SEM_SERVER);

	DB_DATAGRAM *datagram, *new_datagram;

	datagram = (DB_DATAGRAM*)shmem.alloc;
	new_datagram = (DB_DATAGRAM*)malloc(datagram->size);

	memcpy(new_datagram, datagram, datagram->size);

	return new_datagram;

}

#endif

int ipc_listen(int argc, char** args){

	SRVPRINTE("Inicializando IPC de shared memory...\n");

	shmem_init();

	sem_init();
	sem_queue_init();

	//Inicializamos el valor del semaforo de la cola.
	sem_set(shmem.queueid, SEM_QUEUE, 1);
	sem_set(shmem.queueid, SEM_SRV_QUEUE, 0);

	SRVPRINTE("Escuchando clientes...\n");	

	return 0;

}

#ifdef SERVER
void ipc_accept(){

	int pid;

	//Reseteamos los valores de los semaforos para aceptar al nuevo cliente.
	sem_reset();

	//STEP-0: Bloquea hasta que un cliente ejecute semup
	WAIT_FOR(SEM_CLIENT); 

	SRVPRINTE("Cliente conectado...\n");

	cli_count++;

}

int ipc_sync(){
	

	CLIPRINTE("Fork creado, esperando pedido de memoria.\n");

	//STEP-1			
	WAIT_FOR(SEM_CLIENT); //Esperamos al primer mensaje

	DB_DATAGRAM *datagram=(DB_DATAGRAM*)shmem.alloc;

	if(datagram->dg_shmemkey==-1){
		CLIPRINTE("Pedido de zona de memoria recibido.\n");
	}else{
		CLIPRINTE("Error en el protocolo de sincronizacion.\n");
		DUMP_DATAGRAM(datagram);
		exit(1);
	}

	//shmem.alloc tiene el pedido de una zona de memoria. 

	//DUMP_SHMEM_DATA();

	datagram->dg_shmemkey = ftok("../database.sqlite", cli_count); 

	int oldsemid = shmem.semid;

	key_t shmemkey = datagram->dg_shmemkey;

	shmem_detach();

	shmem_init_with_key(shmemkey);
	sem_init_with_key(shmemkey);

	//Reseteamos los nuevos semaforos 
	sem_reset();

	//DUMP_SHMEM_DATA()

	CLIPRINTE("Enviando id de zona de memoria.\n");

	//STEP-2
	//UNBLOCK(SEM_SERVER); //Despertamos el cliente para que haga attach de la zona de memoria pedida
	sem_up(oldsemid,SEM_SERVER);


	//STEP-3
	//WAIT_FOR(SEM_CLIENT); //Esperamos que haga attach, y continuamos para esperar comandos del cliente
	sem_down(oldsemid,SEM_CLIENT);

	CLIPRINTE("Sincronizacion completa. Esperando comandos...\n");

	UNBLOCK_QUEUE(SEM_SRV_QUEUE);

	//PRINT_SEM_VALUES;//Ambos semaforos en 0
	
	return cli_count;

}

void ipc_waitsync(){

	WAIT_FOR_QUEUE(SEM_SRV_QUEUE);

}


#endif

#ifdef CLIENT
int ipc_connect(int argc, char** args){

	DB_DATAGRAM *datagram;

	printf("Conectando con el servidor...\n");

	shmem_init();

	sem_init();
	sem_queue_init();

	printf("Esperando en la cola...\n");

	WAIT_FOR_QUEUE(SEM_QUEUE); //Si entra otro cliente, queda bloqueado en una "cola"

	printf("Conectado con el servidor...\n");

	// STEP-0: Desbloqueamos el servidor
	UNBLOCK(SEM_CLIENT);

	datagram = (DB_DATAGRAM*) shmem.alloc;

	datagram->size = sizeof(DB_DATAGRAM);
	datagram->dg_shmemkey = -1;
	datagram->opcode = OP_CMD;

	//STEP-1
	UNBLOCK(SEM_CLIENT); //Desbloqueamos el servidor

	printf("Pedido de zona de memoria enviado.\n");

	//STEP-2
	WAIT_FOR(SEM_SERVER);//Esperamos que el servidor llene la shmemkey

	printf("Recibiendo zona de memoria...\n");

	//STEP-3
	UNBLOCK(SEM_CLIENT);

	key_t shmemkey = datagram->dg_shmemkey;

	shmem_detach();

	shmem_init_with_key(shmemkey);
	sem_init_with_key(shmemkey);

	//DUMP_SHMEM_DATA();

	UNBLOCK_QUEUE(SEM_QUEUE); //Cuando terminamos de crear la conexion, se libera el servidor
	
	printf("Listo para enviar comandos...\n");

	//PRINT_SEM_VALUES;//Ambos semaforos en 0

	return 0;
}

#endif


void ipc_disconnect(){

	printf("Disconnecting...\n");

#ifdef SERVER
	shmem_destroy();
	sem_destroy();
#else
	shmem_detach();
#endif

}

#ifdef SERVER
void ipc_free(){

	sem_queue_destroy();

}
#endif
