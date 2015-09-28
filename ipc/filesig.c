#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "database.h"
#include "ipc.h"
#include "semaphore.h"

struct session_t {
  int semid;
  int queueid;
  FILE *fp;
  char *filename;
};


char* get_file_name(int pid){
  char aux[16];
  sprintf(aux, "%d%s", pid, ".txt");
  char *dir = calloc(32, sizeof(char));
  sprintf(dir, "files/");
  strcat(dir, aux);
  return dir;
}


ipc_session ipc_newsession() {
  ipc_session session = (ipc_session) malloc(sizeof(struct session_t));
  return (ipc_session)session;
}


int ipc_listen(ipc_session session, int argc, char** args) {
  SRVPRINTE("Inicializando IPC de archivos y señales...\n");

  session->filename = get_file_name(getpid());
  session->fp = fopen(session->filename, "wb");
  fprintf(session->fp, "%s\n", "HOLA");

  sem_init(&session->semid, 2);
  sem_queue_init(&session->queueid, 2);
  // Inicializamos el valor del semaforo de la cola.
  sem_set(session->queueid, SEM_QUEUE, 1);
  sem_set(session->queueid, SEM_SRV_QUEUE, 0);

  SRVPRINTE("Escuchando clientes...\n");
  return 0;
}


void ipc_accept(ipc_session session) {
  sem_reset(session);
  sem_down(session->semid, SEM_CLIENT);
  SRVPRINTE("Cliente conectado...\n");
}


void ipc_sync(ipc_session session) {
  // CLIPRINTE("Fork creado, esperando pedido de memoria.\n");
  // //STEP-1
  // sem_down(session->semid, SEM_CLIENT);
  // DB_DATAGRAM *datagram = (DB_DATAGRAM*)session->fp;
  // if (datagram->opcode == OP_CONNECT) {
  //   CLIPRINTE("Pedido de archivo recibido.\n");
  // } else {
  //   CLIPRINTE("Error en el protocolo de sincronizacion.\n");
  //   DUMP_DATAGRAM(datagram);
  //   exit(1);
  // }
  // datagram->dg_shmemkey = ftok("../database.sqlite", cli_count);

  // int oldsemid = session->semid;

  // key_t shmemkey = datagram->dg_shmemkey;
  // shmem_detach(session);
  // shmem_create_with_key(session, shmemkey);
  // shmem_attach(session);
  // sem_init_with_key(&session->semid, shmemkey, 2);
  // //Reseteamos los nuevos semaforos
  // sem_reset(session);
  // CLIPRINTE("Enviando id de zona de memoria.\n");
  // //STEP-2
  // //UNBLOCK(SEM_SERVER); //Despertamos el cliente para que haga attach de la zona de memoria pedida
  // sem_up(oldsemid, SEM_SERVER);
  // //STEP-3
  // //WAIT_FOR(SEM_CLIENT); //Esperamos que haga attach, y continuamos para esperar comandos del cliente
  // sem_down(oldsemid, SEM_CLIENT);
  // CLIPRINTE("Sincronizacion completa. Esperando comandos...\n");
  // sem_up(session->queueid, SEM_SRV_QUEUE);
}


void ipc_waitsync(ipc_session session) {
  sem_down(session->queueid, SEM_SRV_QUEUE);
}


int ipc_connect(ipc_session session, int argc, char** args) {
  // DB_DATAGRAM *datagram;
  // key_t shmemkey;

  // printf("Conectando con el servidor...\n");

  // shmemkey = ftok("../database.sqlite", 0);
  // shmem_get_with_key(session, shmemkey);
  // shmem_attach(session);

  // sem_init(&session->semid, 2);
  // sem_queue_init(&session->queueid, 2);

  // printf("Esperando en la cola...\n");

  // sem_down(session->queueid, SEM_QUEUE); //Si entra otro cliente, queda bloqueado en una "cola"

  // printf("Conectado con el servidor...\n");

  // // STEP-0: Desbloqueamos el servidor
  // sem_up(session->semid, SEM_CLIENT);

  // datagram = (DB_DATAGRAM*) session->alloc;

  // datagram->size = sizeof(DB_DATAGRAM);
  // datagram->opcode = OP_CONNECT;

  // //STEP-1
  // sem_up(session->semid, SEM_CLIENT); //Desbloqueamos el servidor

  // printf("Pedido de zona de memoria enviado.\n");

  // //STEP-2
  // sem_down(session->semid, SEM_SERVER); //Esperamos que el servidor llene la shmemkey

  // printf("Recibiendo zona de memoria...\n");

  // int oldsemid = session->semid;
  // shmemkey = datagram->dg_shmemkey;

  // //Reseteamos la pagina a 0 para evitar confuciones en el protocolo
  // memset(datagram, 0, SHMEM_SIZE);

  // shmem_detach(session);

  // shmem_get_with_key(session, shmemkey);
  // shmem_attach(session);
  // sem_init_with_key(&session->semid, shmemkey, 2);

  // //STEP-3
  // sem_up(oldsemid, SEM_CLIENT);

  // sem_up(session->queueid, SEM_QUEUE); //Cuando terminamos de crear la conexion, se libera el servidor

  // printf("Listo para enviar comandos...\n");

  // return 0;
}


int ipc_send(ipc_session session, DB_DATAGRAM* data) {
  if (data->size > SHMEM_SIZE) {
    return -1;
  }
  memcpy(session->fp, data, data->size);
  #ifdef SERVER
    sem_up(session->semid, SEM_SERVER);
  #else
    sem_up(session->semid, SEM_CLIENT);
  #endif
  return 0;
}


DB_DATAGRAM* ipc_receive(ipc_session session) {
  #ifdef SERVER
   sem_down(session->semid, SEM_CLIENT);
  #else
   sem_down(session->semid, SEM_SERVER);
  #endif
   DB_DATAGRAM *datagram, *new_datagram;
   datagram = (DB_DATAGRAM*)session->fp;
   new_datagram = (DB_DATAGRAM*)malloc(datagram->size);
   memcpy(new_datagram, datagram, datagram->size);
   return new_datagram;
}


void ipc_disconnect(ipc_session session) {
  printf("Disconnecting...\n");
  #ifdef SERVER
    fclose(session->fp);
    remove(session->filename);
    free(session->filename);
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


bool ipc_shouldfork() {
  return TRUE;
}
