#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "database.h"
#include "ipc.h"

struct session_t {
	int queueid_r;
	int queueid_w;

	int otherpid;
};

struct msgbuf {
	long 			mtype;
	DB_DATAGRAM 	datagram;
};

#ifdef SERVER
extern int cli_count;
#endif

static void msgqueue_init(ipc_session session, int flags) {

	key_t msgqueuekey = ftok("../database.sqlite", 0);

	if ((session->queueid_r = msgget(msgqueuekey, flags | 0666)) == -1)
	{
		SRVPRINTE("msgget failed.\n");
		exit(1);
	}

	SRVPRINT("Message queue de lectura inicializado con id %d\n", session->queueid_r);

	msgqueuekey = ftok("../database.sqlite", 1);

	if ((session->queueid_w = msgget(msgqueuekey, flags | 0666)) == -1)
	{
		SRVPRINTE("msgget failed.\n");
		exit(1);
	}

	SRVPRINT("Message queue de escritura inicializado con id %d\n", session->queueid_w);

}

#ifdef SERVER
static void msgqueue_create(ipc_session session) {

	msgqueue_init(session, IPC_CREAT | IPC_EXCL);

}
#endif

#ifdef CLIENT
static void msgqueue_attach(ipc_session session) {

	msgqueue_init(session, 0);

}
#endif


ipc_session ipc_newsession() {
	ipc_session session = (ipc_session) malloc(sizeof(struct session_t));
	return (ipc_session)session;
}


#ifdef SERVER
int ipc_listen(ipc_session session, int argc, char** args) {

	SRVPRINTE("Inicializando IPC de message queue...\n");

	msgqueue_create(session);

	SRVPRINTE("Escuchando clientes...\n");

	return 0;

}


void ipc_accept(ipc_session session) {

	DB_DATAGRAM* datagram;

	printf("Esperando cliente...\n");

	//Recibimos el pedido de conexion
	datagram = ipc_receive(session);

	if (datagram->opcode != OP_CONNECT) {
		CLIPRINTE("Error en el protocolo de sincronizacion.\n");
		DUMP_DATAGRAM(datagram);
		exit(1);
	}

	free(datagram);

	printf("Cliente conectado...\n");

}


void ipc_sync(ipc_session session) {

}


void ipc_waitsync(ipc_session session) {

}
#endif

#ifdef CLIENT
int ipc_connect(ipc_session session, int argc, char** args) {

	DB_DATAGRAM *datagram;

	printf("Conectando con el servidor...\n");

	msgqueue_attach(session);

	datagram = malloc(sizeof(DB_DATAGRAM));
	datagram->size = sizeof(DB_DATAGRAM);
	datagram->opcode = OP_CONNECT;

	ipc_send(session, datagram);
	free(datagram);

	return 0;
}
#endif

int ipc_send(ipc_session session, DB_DATAGRAM* data) {

	struct msgbuf *message;

	message = malloc(data->size + sizeof(long));

#ifdef SERVER
	message->mtype = data->sender;
#else
	message->mtype = getpid();
#endif

	memcpy(&message->datagram, data, data->size);

	//DUMP_DATAGRAM((&message->datagram));

#ifdef SERVER
	msgsnd(session->queueid_w, message, data->size, 0);
#else
	msgsnd(session->queueid_r, message, data->size, 0);
#endif

	return 0;

}

DB_DATAGRAM* ipc_receive(ipc_session session) {

	struct msgbuf *ans;
	DB_DATAGRAM *datagram;

	ans = malloc(DATAGRAM_MAXSIZE + sizeof(long));

#ifdef SERVER
	msgrcv(session->queueid_r, ans, DATAGRAM_MAXSIZE, 0,  0);
#else
	msgrcv(session->queueid_w, ans, DATAGRAM_MAXSIZE, getpid(),  0);
#endif

	datagram = malloc(ans->datagram.size);
	memcpy(datagram, &ans->datagram, ans->datagram.size);

	//DUMP_DATAGRAM(datagram);

#ifdef SERVER
	datagram->sender = ans->mtype;
#endif

	free(ans);

	return datagram;

}


void ipc_disconnect(ipc_session session) {

#ifdef SERVER
	msgctl(session->queueid_r, IPC_RMID, 0);
	msgctl(session->queueid_w, IPC_RMID, 0);
#endif

}

void ipc_free(ipc_session session) {

	free(session);

}

bool ipc_shouldfork() {
	return FALSE;
}
