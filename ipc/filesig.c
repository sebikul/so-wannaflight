#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "database.h"
#include "ipc.h"
//#include "semaphore.h"

struct session_t {
  int semid;
  int queueid;
  FILE *fp;
};


ipc_session ipc_newsession() {
  ipc_session session = (ipc_session) malloc(sizeof(struct session_t));
  return (ipc_session)session;
}


int ipc_listen(ipc_session session, int argc, char** args) {
  SRVPRINTE("Inicializando IPC de archivos y señales...\n");
  session->fp = fopen("server.txt", "wb");
  fprintf(session->fp, "%s\n", "HOLA");
  fclose(session->fp);

  //sem_init(&session->semid, 2);
  //sem_queue_init(&session->queueid, 2);
  //Inicializamos el valor del semaforo de la cola.
  //sem_set(session->queueid, SEM_QUEUE, 1);
  //sem_set(session->queueid, SEM_SRV_QUEUE, 0);

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

}


DB_DATAGRAM* ipc_receive(ipc_session session) {

}


void ipc_disconnect(ipc_session session) {

}


void ipc_free(ipc_session session) {

}


bool ipc_shouldfork() {
  return TRUE;
}
