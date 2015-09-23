#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "config.h"
#include "database.h"
#include "ipc.h"

struct session_t {
	int queueid;

	int otherpid;

};

struct msgbuf {
	long 			mtype;
	DB_DATAGRAM 	datagram;
};

static void msgqueue_init(ipc_session session, int flags) {

	key_t msgqueuekey = ftok("../database.sqlite", 0);

	if ((session->queueid = msgget(msgqueuekey, flags | 0666)) == -1)
	{
		SRVPRINTE("msgget failed.\n");
		exit(1);
	}

	SRVPRINT("Message queue inicializado con id %d\n", session->queueid);

}

static void msgqueue_create(ipc_session session) {

	msgqueue_init(session, IPC_CREAT | IPC_EXCL);

}

static void msgqueue_attach(ipc_session session) {

	msgqueue_init(session, 0);

}


ipc_session ipc_newsession() {
	ipc_session session = (ipc_session) malloc(sizeof(struct session_t));
	return (ipc_session)session;
}


int ipc_listen(ipc_session session, int argc, char** args) {

	SRVPRINTE("Inicializando IPC de message queue...\n");

	msgqueue_create(session);

	SRVPRINTE("Escuchando clientes...\n");

	return 0;

}


void ipc_accept(ipc_session session) {

}


void ipc_sync(ipc_session session) {

}


void ipc_waitsync(ipc_session session) {

}


int ipc_connect(ipc_session session, int argc, char** args) {

}


int ipc_send(ipc_session session, DB_DATAGRAM* data) {

	struct msgbuf *message;

	message = malloc(data->size + sizeof(long) + sizeof(int));
	message->mtype = session->otherpid;
	memcpy(&message->datagram, data, data->size);

	msgsnd(session->queueid, message, data->size, 0));

	return 0;

}


DB_DATAGRAM* ipc_receive(ipc_session session) {

	struct msgbuf *ans;

	ans = malloc(DATAGRAM_MAXSIZE + );

	msgrcv(session->queueid, ans, DATAGRAM_MAXSIZE, session->otherpid,  0));

	ans = realloc(ans, ans->size)

}


void ipc_disconnect(ipc_session session) {

}


void ipc_free(ipc_session session) {

}
