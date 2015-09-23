#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#include "config.h"
#include "database.h"
#include "ipc.h"
#include "semaphore.h"

struct session_t {

	int semid;
	int queueid;

	int memid;
	char* alloc;
};

#define DUMP_SHMEM_DATA() 	printf("\n[\nsemid: %d\nqueueid: %d\nmemid: %d\nalloc: %p\n]\n\n", session->semid, session->queueid, session->memid,session->alloc);

#ifdef SERVER
extern int cli_count;
#endif

static void shmem_init_with_key(ipc_session session, key_t shmemkey) {

	if ( (session->memid = shmget(shmemkey, SHMEM_SIZE, IPC_CREAT | 0666)) == -1 ) {
		SRVPRINTE("shmget failed.\n");
		exit(1);
	}

	if ( !(session->alloc = shmat(session->memid, NULL, 0)) ) {
		SRVPRINTE("shmget failed.\n");
		exit(1);
	}

	memset(session->alloc, 0, SHMEM_SIZE);

	SRVPRINT("Shared memory inicializado con id %d\n", session->memid);

}

/*
	Crea la zona de memoria compartida
 */
static void shmem_init(ipc_session session) {

	key_t shmemkey = ftok("../database.sqlite", 0);

	shmem_init_with_key(session, shmemkey);

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
#endif

ipc_session ipc_newsession() {

	ipc_session session = (ipc_session) malloc(sizeof(struct session_t));

	return (ipc_session)session;

}

DB_DATAGRAM* ipc_receive(ipc_session session) {

#ifdef SERVER
	sem_down(session->semid, SEM_CLIENT); //Esperamos que el cliente mande un comando
#else
	sem_down(session->semid, SEM_SERVER); //Esperamos que el servidor mande una respuesta
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
	sem_up(session->semid, SEM_SERVER); //Desbloqueamos el cliente
#else
	sem_up(session->semid, SEM_CLIENT); //Desbloqueamos el servidor
#endif

	return 0;

}


#ifdef SERVER
int ipc_listen(ipc_session session, int argc, char** args) {

	SRVPRINTE("Inicializando IPC de shared memory...\n");

	shmem_init(session);

	sem_init(&session->semid, 2);
	sem_queue_init(&session->queueid, 2);

	//Inicializamos el valor del semaforo de la cola.
	sem_set(session->queueid, SEM_QUEUE, 1);
	sem_set(session->queueid, SEM_SRV_QUEUE, 0);

	SRVPRINTE("Escuchando clientes...\n");

	return 0;

}

void ipc_accept(ipc_session session) {

	//Reseteamos los valores de los semaforos para aceptar al nuevo cliente.
	sem_reset(session);

	//STEP-0: Bloquea hasta que un cliente ejecute semup
	//PRINT_SEM_VALUES;
	sem_down(session->semid, SEM_CLIENT);

	SRVPRINTE("Cliente conectado...\n");

}

void ipc_sync(ipc_session session) {

	CLIPRINTE("Fork creado, esperando pedido de memoria.\n");

	//STEP-1
	sem_down(session->semid, SEM_CLIENT); //Esperamos al primer mensaje

	DB_DATAGRAM *datagram = (DB_DATAGRAM*)session->alloc;

	if (datagram->opcode == OP_CONNECT) {
		CLIPRINTE("Pedido de zona de memoria recibido.\n");
	} else {
		CLIPRINTE("Error en el protocolo de sincronizacion.\n");
		DUMP_DATAGRAM(datagram);
		exit(1);
	}

	//shmem.alloc tiene el pedido de una zona de memoria.

	datagram->dg_shmemkey = ftok("../database.sqlite", cli_count);

	int oldsemid = session->semid;

	key_t shmemkey = datagram->dg_shmemkey;

	shmem_detach(session);

	shmem_init_with_key(session, shmemkey);
	sem_init_with_key(&session->semid, shmemkey, 2);

	//Reseteamos los nuevos semaforos
	sem_reset(session);

	CLIPRINTE("Enviando id de zona de memoria.\n");

	//STEP-2
	//UNBLOCK(SEM_SERVER); //Despertamos el cliente para que haga attach de la zona de memoria pedida
	sem_up(oldsemid, SEM_SERVER);

	//STEP-3
	//WAIT_FOR(SEM_CLIENT); //Esperamos que haga attach, y continuamos para esperar comandos del cliente
	sem_down(oldsemid, SEM_CLIENT);

	CLIPRINTE("Sincronizacion completa. Esperando comandos...\n");

	sem_up(session->queueid, SEM_SRV_QUEUE);


}

void ipc_waitsync(ipc_session session) {

	sem_down(session->queueid, SEM_SRV_QUEUE);

}

#endif

#ifdef CLIENT
int ipc_connect(ipc_session session, int argc, char** args) {

	DB_DATAGRAM *datagram;

	printf("Conectando con el servidor...\n");

	shmem_init(session);

	sem_init(&session->semid, 2);
	sem_queue_init(&session->queueid, 2);

	printf("Esperando en la cola...\n");

	sem_down(session->queueid, SEM_QUEUE); //Si entra otro cliente, queda bloqueado en una "cola"

	printf("Conectado con el servidor...\n");

	// STEP-0: Desbloqueamos el servidor
	sem_up(session->semid, SEM_CLIENT);

	datagram = (DB_DATAGRAM*) session->alloc;

	datagram->size = sizeof(DB_DATAGRAM);
	datagram->opcode = OP_CONNECT;

	//STEP-1
	sem_up(session->semid, SEM_CLIENT); //Desbloqueamos el servidor

	printf("Pedido de zona de memoria enviado.\n");

	//STEP-2
	sem_down(session->semid, SEM_SERVER); //Esperamos que el servidor llene la shmemkey

	printf("Recibiendo zona de memoria...\n");

	int oldsemid = session->semid;
	key_t shmemkey = datagram->dg_shmemkey;

	//Reseteamos la pagina a 0 para evitar confuciones en el protocolo
	memset(datagram, 0, SHMEM_SIZE);

	shmem_detach(session);

	shmem_init_with_key(session, shmemkey);
	sem_init_with_key(&session->semid, shmemkey, 2);

	//STEP-3
	sem_up(oldsemid, SEM_CLIENT);

	sem_up(session->queueid, SEM_QUEUE); //Cuando terminamos de crear la conexion, se libera el servidor

	printf("Listo para enviar comandos...\n");

	return 0;
}

#endif

void ipc_disconnect(ipc_session session) {

	printf("Disconnecting...\n");

#ifdef SERVER
	shmem_destroy(session);
	sem_destroy(session->semid);
#else
	shmem_detach(session);
#endif

}


void ipc_free(ipc_session session) {

#ifdef SERVER
	sem_queue_destroy(session->queueid);
#endif

	free(session);

}

bool ipc_shouldfork(){
	return TRUE;
}
