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

#define PRINT_SEM_VALUES 	printf("SEM_CLIENT: %d\nSEM_SERVER: %d\n", get_sem_val(session->semid, SEM_CLIENT), get_sem_val(session->emid, SEM_SERVER))
#define PRINT_QSEM_VALUES 	printf("SEM_QUEUE: %d\nSEM_SRV_QUEUE: %d\n", get_sem_val(session->queueid, SEM_QUEUE), get_sem_val(session->queueid, SEM_SRV_QUEUE))


#define SEM_QUEUE			0
#define SEM_SRV_QUEUE		1

#define SEM_SERVER 			0
#define SEM_CLIENT 			1

#define WAIT_FOR(sem) 		sem_down(session->semid, sem)
#define UNBLOCK(sem)		sem_up(session->semid, sem)

#define WAIT_FOR_QUEUE(sem) sem_down(session->queueid, sem)
#define UNBLOCK_QUEUE(sem) 	sem_up(session->queueid, sem)


struct session_t{

	int clientid;

	int semid;
	int queueid;

	int memid;
	char* alloc;
};

#define DUMP_SHMEM_DATA() 	printf("\n[\nsemid: %d\nqueueid: %d\nmemid: %d\nalloc: %p\n]\n\n", shmem.semid, shmem.queueid, shmem.memid,shmem.alloc);

#ifdef SERVER
extern int cli_count;
#endif

static void shmem_init_with_key(ipc_session session, key_t shmemkey) {

	if ( (session->memid = shmget(shmemkey, SHMEM_SIZE, IPC_CREAT | 0666)) == -1 ) {
		printf("shmget failed.\n");
		exit(1);
	}

	if ( !(session->alloc = shmat(session->memid, NULL, 0)) ) {
		printf("shmget failed.\n");
		exit(1);
	}

	memset(session->alloc, 0, SHMEM_SIZE);

	//printf("Initialized shared memory zone with id %d\n", shmem.memid);

}

/*
	Crea la zona de memoria compartida
 */
static void shmem_init(ipc_session session) {

	key_t shmemkey = ftok("../database.sqlite", 0);

	shmem_init_with_key(session, shmemkey);

}

static void sem_init_with_key(ipc_session session, key_t semkey) {

	//Creamos los semaforos para sincronizar cliente servidor
	if ((session->semid = semget(semkey, 2, 0)) >= 0 ) {
		//printf("Sync semaphores already exist with id: %d\n", shmem.semid);
		return;
	}

	if ((session->semid = semget(semkey, 2, IPC_CREAT | 0666)) == -1 ) {
		printf("shmget failed.\n");
		exit(1);
	}

	//printf("Initialized sync semaphores with id %d\n", shmem.semid);

}

/*
	Inicializa 2 semaforos para sincronizar cliente-servidor
 */
static void sem_init(ipc_session session) {

	key_t semkey = ftok("../database.sqlite", 0);

	sem_init_with_key(session, semkey);

}

static void sem_queue_init(ipc_session session) {

	//El id solo usa los 8 bits menos significativos. Seteamos a FF que es la cant maxima de clientes -1 que puede aguantar el servidor
	key_t queuekey = ftok("../database.sqlite", 0xFF);

	//Creamos el semaforo para la cola de clientes
	if ((session->queueid = semget(queuekey, 2, 0)) >= 0 ) {
		//printf("Queue semaphore already exist with id: %d\n", shmem.queueid);
		return;
	}

	if ((session->queueid = semget(queuekey, 2, IPC_CREAT | 0666)) == -1 ) {
		printf("shmget queue failed.\n");
		exit(1);
	}

	//printf("Initialized queue semaphores with id %d\n", shmem.queueid);


}

/*
	Setea el valor del semaforo _sem_ a _val_
 */
static void sem_set(int semid, int semnum, int val) {

	semctl(semid, semnum, SETVAL, val);

}

static void shmem_detach(ipc_session session) {

	printf("Detaching shared memory zone...\n");

	shmdt(session->alloc);

}

#ifdef SERVER
static void sem_reset(ipc_session session) {

	sem_set(session->semid, SEM_CLIENT, 0);
	sem_set(session->semid, SEM_SERVER, 0);

}

/*
	Elimina la zona de memoria compartida.
 */
static void shmem_destroy(ipc_session session) {

	shmem_detach(session);

	CLIPRINTE("Destroying shared memory zone...\n");

	//Marcamos la memoria como eliminable
	shmctl(session->memid, IPC_RMID, 0);

}

static void sem_destroy(ipc_session session) {

	CLIPRINTE("Destroying semaphores...\n");

	//Destruimos los semaforos
	semctl(session->semid, 0, IPC_RMID);

}

static void sem_queue_destroy(ipc_session session) {

	semctl(session->queueid, 0, IPC_RMID);

}

#endif
/*
	Ejecuta down sobre el semaforo
 */
static void sem_down(int semid, int semnum) {

	struct sembuf sb;

	sb.sem_num = semnum;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;
	semop(semid, &sb, 1);
}

/*
	Ejecuta un up sobre el semaforo
 */
static void sem_up(int semid, int semnum) {

	struct sembuf sb;

	sb.sem_num = semnum;
	sb.sem_op = 1;
	sb.sem_flg = SEM_UNDO;
	semop(semid, &sb, 1);
}

static int get_sem_val(int semid, int semnum ) {
	return (semctl(semid, semnum, GETVAL));
}

ipc_session ipc_newsession(){

	ipc_session session = (ipc_session) malloc(sizeof(struct session_t));

	return (ipc_session)session;

}

DB_DATAGRAM* ipc_receive(ipc_session session) {

#ifdef SERVER
	WAIT_FOR(SEM_CLIENT); //Esperamos que el cliente mande un comando
#else
	WAIT_FOR(SEM_SERVER); //Esperamos que el servidor mande una respuesta
#endif


	DB_DATAGRAM *datagram, *new_datagram;

	datagram = (DB_DATAGRAM*)session->alloc;
	new_datagram = (DB_DATAGRAM*)malloc(datagram->size);

	memcpy(new_datagram, datagram, datagram->size);

	return new_datagram;

}

int ipc_send(ipc_session session, DB_DATAGRAM* data) {

	if (data->size > SHMEM_SIZE) {
		return -1;
	}

	memcpy(session->alloc, data, data->size);


#ifdef SERVER
	UNBLOCK(SEM_SERVER); //Desbloqueamos el cliente
#else
	UNBLOCK(SEM_CLIENT); //Desbloqueamos el servidor
#endif

	return 0;

}


int ipc_listen(ipc_session session, int argc, char** args) {

	SRVPRINTE("Inicializando IPC de shared memory...\n");

	shmem_init(session);

	sem_init(session);
	sem_queue_init(session);

	//Inicializamos el valor del semaforo de la cola.
	sem_set(session->queueid, SEM_QUEUE, 1);
	sem_set(session->queueid, SEM_SRV_QUEUE, 0);

	SRVPRINTE("Escuchando clientes...\n");

	return 0;

}

#ifdef SERVER
void ipc_accept(ipc_session session) {

	//Reseteamos los valores de los semaforos para aceptar al nuevo cliente.
	sem_reset(session);

	//STEP-0: Bloquea hasta que un cliente ejecute semup
	WAIT_FOR(SEM_CLIENT);

	SRVPRINTE("Cliente conectado...\n");

}

void ipc_sync(ipc_session session) {


	CLIPRINTE("Fork creado, esperando pedido de memoria.\n");

	//STEP-1
	WAIT_FOR(SEM_CLIENT); //Esperamos al primer mensaje

	DB_DATAGRAM *datagram = (DB_DATAGRAM*)session->alloc;

	if (datagram->dg_shmemkey == -1) {
		CLIPRINTE("Pedido de zona de memoria recibido.\n");
	} else {
		CLIPRINTE("Error en el protocolo de sincronizacion.\n");
		DUMP_DATAGRAM(datagram);
		exit(1);
	}

	//shmem.alloc tiene el pedido de una zona de memoria.

	//DUMP_SHMEM_DATA();

	datagram->dg_shmemkey = ftok("../database.sqlite", cli_count);

	int oldsemid = session->semid;

	key_t shmemkey = datagram->dg_shmemkey;

	shmem_detach(session);

	shmem_init_with_key(session, shmemkey);
	sem_init_with_key(session, shmemkey);

	//Reseteamos los nuevos semaforos
	sem_reset(session);

	//DUMP_SHMEM_DATA()

	CLIPRINTE("Enviando id de zona de memoria.\n");

	//STEP-2
	//UNBLOCK(SEM_SERVER); //Despertamos el cliente para que haga attach de la zona de memoria pedida
	sem_up(oldsemid, SEM_SERVER);


	//STEP-3
	//WAIT_FOR(SEM_CLIENT); //Esperamos que haga attach, y continuamos para esperar comandos del cliente
	sem_down(oldsemid, SEM_CLIENT);

	CLIPRINTE("Sincronizacion completa. Esperando comandos...\n");

	UNBLOCK_QUEUE(SEM_SRV_QUEUE);

	//PRINT_SEM_VALUES;//Ambos semaforos en 0

}

void ipc_waitsync(ipc_session session) {

	WAIT_FOR_QUEUE(SEM_SRV_QUEUE);

}


#endif

#ifdef CLIENT
int ipc_connect(ipc_session session, int argc, char** args) {

	DB_DATAGRAM *datagram;

	printf("Conectando con el servidor...\n");

	shmem_init(session);

	sem_init(session);
	sem_queue_init(session);

	printf("Esperando en la cola...\n");

	WAIT_FOR_QUEUE(SEM_QUEUE); //Si entra otro cliente, queda bloqueado en una "cola"

	printf("Conectado con el servidor...\n");

	// STEP-0: Desbloqueamos el servidor
	UNBLOCK(SEM_CLIENT);

	datagram = (DB_DATAGRAM*) session->alloc;

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

	shmem_detach(session);

	shmem_init_with_key(session, shmemkey);
	sem_init_with_key(session, shmemkey);

	//DUMP_SHMEM_DATA();

	UNBLOCK_QUEUE(SEM_QUEUE); //Cuando terminamos de crear la conexion, se libera el servidor

	printf("Listo para enviar comandos...\n");

	//PRINT_SEM_VALUES;//Ambos semaforos en 0

	return 0;
}

#endif


void ipc_disconnect(ipc_session session) {

	printf("Disconnecting...\n");

#ifdef SERVER
	shmem_destroy(session);
	sem_destroy(session);
#else
	shmem_detach(session);
#endif

}


void ipc_free(ipc_session session) {

#ifdef SERVER
	sem_queue_destroy(session);
#endif

	free(session)

}

