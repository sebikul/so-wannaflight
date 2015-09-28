#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "database.h"
#include "ipc.h"

struct session_t {
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
  return (ipc_session) malloc(sizeof(struct session_t));
}


int ipc_listen(ipc_session session, int argc, char** args) {
  SRVPRINTE("Inicializando IPC de archivos y señales...\n");
  session->filename = get_file_name(getpid());
  session->fp = fopen(session->filename, "wb");
  // TODO
  SRVPRINTE("Escuchando clientes...\n");
  return 0;
}


void ipc_accept(ipc_session session) {
  // TODO
  SRVPRINTE("Cliente conectado...\n");
}


void ipc_sync(ipc_session session) {
  // TODO
}


void ipc_waitsync(ipc_session session) {
  // TODO
}


int ipc_connect(ipc_session session, int argc, char** args) {
  // TODO
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
  // TODO
}


void ipc_free(ipc_session session) {
  fclose(session->fp);
  remove(session->filename);
  free(session->filename);
  free(session);
}


bool ipc_shouldfork() {
  return TRUE;
}
