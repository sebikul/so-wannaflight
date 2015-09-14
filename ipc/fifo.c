#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "database.h"
#include "ipc.h"

struct session_t {
	char* path;

	int serverfd_r;
	int serverfd_w;
};

#ifdef SERVER
extern int cli_count;
#endif

void sigpipe_handler(int s) {
	printf("Recibido SIGPIPE.\n");
	// ipc_disconnect(session);
	// ipc_free(session);
	exit(0);
}

static int file_exist(const char* path) {
	return (access(path, 0) == 0);
}

static int make_fifo(const char* path) {
	return mknod(path, S_IFIFO | 0666, 0);
}

ipc_session ipc_newsession() {

	ipc_session session = (ipc_session) malloc(sizeof(struct session_t));

	return (ipc_session)session;

}

#ifdef SERVER
int ipc_listen(ipc_session session, int argc, char** args) {

	printf("Creando FIFO...\n");

	printf("Abriendo FIFO para lectura...\n");

	if ( !file_exist(FIFO_INITIAL_PATH "-r") && make_fifo(FIFO_INITIAL_PATH "-r") == -1 ) {
		fprintf(stderr, "Error al crear el FIFO en " FIFO_INITIAL_PATH "-r" "\n");
	}

	printf("Abriendo FIFO para escritura...\n");

	if ( !file_exist(FIFO_INITIAL_PATH "-w") && make_fifo(FIFO_INITIAL_PATH "-w") == -1 ) {
		fprintf(stderr, "Error al crear el FIFO en " FIFO_INITIAL_PATH "-w" "\n");
	}

	signal(SIGPIPE, sigpipe_handler);

	return 0;

}

void ipc_accept(ipc_session session) {

	session->serverfd_r = open(FIFO_INITIAL_PATH "-r", O_RDONLY);
	session->serverfd_w = open(FIFO_INITIAL_PATH "-w", O_WRONLY);

	printf("Esperando a que un cliente escriba...\n");


	}

void ipc_sync(ipc_session session) {

	int n, oldsrvfd_w;
	char newpath[64];
	DB_DATAGRAM* datagram;

	datagram = (DB_DATAGRAM*) calloc(DATAGRAM_MAXSIZE, sizeof(char));

	//El servidor se bloquea aca.
	n = read(session->serverfd_r, datagram, DATAGRAM_MAXSIZE);

	if(datagram->opcode!=OP_CONNECT){
		CLIPRINTE("Error en el protocolo de sincronizacion.\n");
		DUMP_DATAGRAM(datagram);
		exit(1);
	}

	printf("Cliente conectado...\n");

	oldsrvfd_w = session->serverfd_w;

	printf("Creando nuevo FIFO de lectura\n");
	close(FIFO_INITIAL_PATH "-r");
	sprintf(newpath, "%s-%d-r", FIFO_INITIAL_PATH, cli_count);
	if ( !file_exist(newpath) && make_fifo(newpath) == -1 ) {
		fprintf(stderr, "Error al crear el FIFO en %s\n", newpath);
	}
	session->serverfd_r = open(newpath, O_RDWR);

	printf("Creando nuevo FIFO de escritura\n");
	close(FIFO_INITIAL_PATH "-w");
	sprintf(newpath, "%s-%d-w", FIFO_INITIAL_PATH, cli_count);
	if ( !file_exist(newpath) && make_fifo(newpath) == -1 ) {
		fprintf(stderr, "Error al crear el FIFO en %s\n", newpath);
	}
	session->serverfd_w = open(newpath, O_RDWR);

	sprintf(newpath, "%s-%d", FIFO_INITIAL_PATH, cli_count);

	strcpy(datagram->dg_cmd, newpath);
	datagram->size = sizeof(DB_DATAGRAM) + strlen(datagram->dg_cmd);

	write(oldsrvfd_w, datagram, datagram->size);
	free(datagram);

	datagram = malloc(DATAGRAM_MAXSIZE);

	printf("Esperando ACK\n");

	n = read(session->serverfd_r, datagram, DATAGRAM_MAXSIZE);

	printf("Cliente sincronizado! %s\n", datagram->dg_cmd);

	// int oldfd = session->serverfd_w;

	// session->serverfd_w = open(FIFO_INITIAL_PATH, O_WRONLY);

}

void ipc_waitsync(ipc_session session) {

	exit(0);

}
#endif

#ifdef CLIENT
int ipc_connect(ipc_session session, int argc, char** args) {

	int n;
	char newpath[64];

	DB_DATAGRAM* datagram;

	if ( !file_exist(FIFO_INITIAL_PATH "-r")) {
		fprintf(stderr, "Error al conectarse con el FIFO en de escritura " FIFO_INITIAL_PATH "-r" "\n");
		exit(1);
	}

	if ( !file_exist(FIFO_INITIAL_PATH "-w")) {
		fprintf(stderr, "Error al conectarse con el FIFO en de lectura " FIFO_INITIAL_PATH "-w" "\n");
		exit(1);
	}

	session->serverfd_r = open(FIFO_INITIAL_PATH "-r", O_WRONLY);
	session->serverfd_w = open(FIFO_INITIAL_PATH "-w", O_RDONLY);

	datagram = (DB_DATAGRAM*) calloc(DATAGRAM_MAXSIZE, sizeof(char));
	datagram->opcode = OP_CONNECT;
	datagram->size=sizeof(DB_DATAGRAM);

	DUMP_DATAGRAM(datagram);

	printf("Escribiendo pedido de conexion...\n");

	write(session->serverfd_r, datagram, datagram->size);
	free(datagram);

	printf("Esperando nuevo FIFO\n");

	datagram = malloc(DATAGRAM_MAXSIZE);

	//El cliente se bloquea aca.
	n = read(session->serverfd_w, datagram, DATAGRAM_MAXSIZE);

	printf("Nuevo FIFO: %s\n", datagram->dg_cmd);

	close(session->serverfd_r);
	close(session->serverfd_w);

	sprintf(newpath, "%s-r", datagram->dg_cmd);
	session->serverfd_r = open(newpath, O_WRONLY);

	sprintf(newpath, "%s-w", datagram->dg_cmd);
	session->serverfd_w = open(newpath, O_RDONLY);

	strcpy(datagram->dg_cmd, "Hola!!!!");
	datagram->size = sizeof(DB_DATAGRAM) + strlen(datagram->dg_cmd);

	write(session->serverfd_r, datagram, datagram->size);
	free(datagram);

	return 0;

}
#endif

int ipc_send(ipc_session session, DB_DATAGRAM* data) {



}

DB_DATAGRAM* ipc_receive(ipc_session session) {


}

void ipc_disconnect(ipc_session session) {



}

void ipc_free(ipc_session session) {

	free(session);

}
