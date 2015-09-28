#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "config.h"
#include "database.h"
#include "ipc.h"

struct session_t {
  FILE *server_file;
  FILE *requests_file;
};

static struct sigaction sig;
int waiting = 0;
int req_pos = 0;
int client_pid;

char* get_file_name(int pid){
  char aux[16];
  sprintf(aux, "%d%s", pid, ".txt");
  char *dir = calloc(32, sizeof(char));
  sprintf(dir, "./files/");
  strcat(dir, aux);
  return dir;
}


ipc_session ipc_newsession() {
  return (ipc_session) malloc(sizeof(struct session_t));
}


void request_received(int sig){
  if(sig == SIGUSR1){
    waiting = 0;
  }
}


int ipc_listen(ipc_session session, int argc, char** args) {
  SRVPRINTE("Inicializando IPC de archivos y señales...\n");
  session->server_file = fopen("./files/server_file", "w");
  session->requests_file = fopen("./files/requests_file", "w");
  // TODO ESCRIBIR EN server_file EL PID DEL SERVER
  char c = 1;
  lseek(session->server_file, 1024*1024, SEEK_SET);
  write(session->server_file, &c, 1);
  lseek(session->requests_file, 1024*1024, SEEK_SET);
  write(session->requests_file, &c, 1);

  signal(SIGUSR1, request_received);

  waiting = 1;

  SRVPRINTE("Escuchando clientes...\n");
  return 0;
}


void ipc_accept(ipc_session session) {
  while(waiting){
    pause();
  }
  waiting = 1;
  signal(SIGUSR1, request_received);

  SRVPRINTE("Cliente conectado...\n");
}


void ipc_sync(ipc_session session) {

}


void ipc_waitsync(ipc_session session) {

}


int ipc_connect(ipc_session session, int argc, char** args) {
  int server_pid;
  read(session->server_file, &server_pid, sizeof(int));
  kill(server_pid, SIGUSR1);
  return 0;
}


int ipc_send(ipc_session session, DB_DATAGRAM* data) {
  
  return 0;
}


DB_DATAGRAM* ipc_receive(ipc_session session) {
  while(waiting){
    pause();
  }
  waiting = 1;
  signal(SIGUSR1, request_received);

  DB_DATAGRAM *datagram = (DB_DATAGRAM)malloc(sizeof(DB_DATAGRAM));
  lseek(session->requests_file, req_pos, SEEK_SET);
  read(session->requests_file, &datagram, sizeof(DB_DATAGRAM));
  return datagram;
}


void ipc_disconnect(ipc_session session) {
  printf("Disconnecting...\n");
}


void ipc_free(ipc_session session) {
  fclose(session->server_file);
  fclose(session->requests_file);
  remove("./files/server_file");
  remove("./files/requests_file");
  free(session);
}


bool ipc_shouldfork() {
  return TRUE;
}
