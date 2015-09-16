#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>

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

#ifdef SERVER
static int make_fifo(const char* path) {
	return mknod(path, S_IFIFO | 0666, 0);
}
#endif

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

	session->serverfd_r = open(FIFO_INITIAL_PATH "-r", O_RDONLY);
	session->serverfd_w = open(FIFO_INITIAL_PATH "-w", O_WRONLY);

	return 0;

}

void ipc_accept(ipc_session session) {

	DB_DATAGRAM* datagram;

	printf("Esperando cliente...\n");

	//flock(session->serverfd_r, LOCK_EX);

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

	char newpath_r[64];
	char newpath_w[64];
	char newpath_base[64];
	DB_DATAGRAM* datagram;

	sprintf(newpath_r, "%s-%d-r", FIFO_INITIAL_PATH, cli_count);
	printf("Creando nuevo FIFO de lectura: %s\n", newpath_r);
	if ( !file_exist(newpath_r) && make_fifo(newpath_r) == -1 ) {
		fprintf(stderr, "Error al crear el FIFO en %s\n", newpath_r);
		exit(1);
	}

	sprintf(newpath_w, "%s-%d-w", FIFO_INITIAL_PATH, cli_count);
	printf("Creando nuevo FIFO de escritura: %s\n", newpath_w);
	if ( !file_exist(newpath_w) && make_fifo(newpath_w) == -1 ) {
		fprintf(stderr, "Error al crear el FIFO en %s\n", newpath_w);
		exit(1);
	}

	datagram = malloc(DATAGRAM_MAXSIZE);

	//A esta altura los FIFO existen en el fs, pero nadie esta conectado. Falta mandarselos
	//al cliente, y luego reemplazar los actuales para poder aceptar al proximo cliente.

	//Creamos el prefijo del path para pasarselo al cliente y que lo use
	//para conectarse a los nuevos FIFO.
	sprintf(newpath_base, "%s-%d", FIFO_INITIAL_PATH, cli_count);
	strcpy(datagram->dg_cmd, newpath_base);
	datagram->size = sizeof(DB_DATAGRAM) + strlen(datagram->dg_cmd);

	DUMP_DATAGRAM(datagram);

	ipc_send(session, datagram);
	free(datagram);

	printf("Esperando ACK\n");

	//flock(session->serverfd_r, LOCK_UN);

	close(session->serverfd_w);
	close(session->serverfd_r);

	//Bloquea en el open hasta que el cliente se conecte al nuevo FIFO.
	printf("Abriendo nuevo FIFO de escritura: %s\n", newpath_w);
	session->serverfd_w = open(newpath_w, O_WRONLY);

	printf("Abriendo nuevo FIFO de lectura: %s\n", newpath_r);
	session->serverfd_r = open(newpath_r, O_RDONLY);

	datagram = ipc_receive(session);

	DUMP_DATAGRAM(datagram);

	printf("Cliente sincronizado! %s\n", datagram->dg_cmd);
	free(datagram);

}

void ipc_waitsync(ipc_session session) {

	sleep(50000);

}
#endif

#ifdef CLIENT
int ipc_connect(ipc_session session, int argc, char** args) {

	char newpath_r[64];
	char newpath_w[64];

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

	printf("Escribiendo pedido de conexion...\n");

	datagram = (DB_DATAGRAM*) malloc(DATAGRAM_MAXSIZE);
	datagram->opcode = OP_CONNECT;
	datagram->size = sizeof(DB_DATAGRAM);

	DUMP_DATAGRAM(datagram);

	ipc_send(session, datagram);
	free(datagram);
	//El servidor se desbloquea

	printf("Esperando nuevo FIFO\n");

	datagram = ipc_receive(session);

	printf("Nuevo FIFO a usar: %s\n", datagram->dg_cmd);

	sprintf(newpath_r, "%s-r", datagram->dg_cmd);
	sprintf(newpath_w, "%s-w", datagram->dg_cmd);

	close(session->serverfd_r);
	close(session->serverfd_w);

	printf("Abriendo nuevo FIFO de lectura: %s\n", newpath_w);
	session->serverfd_w = open(newpath_w, O_RDONLY);

	printf("Abriendo nuevo FIFO de escritura: %s\n", newpath_r);
	session->serverfd_r = open(newpath_r, O_WRONLY);


	strcpy(datagram->dg_cmd, "Hola!!!!");
	datagram->size = sizeof(DB_DATAGRAM) + strlen(datagram->dg_cmd);
	datagram->opcode=OP_CMD;

	DUMP_DATAGRAM(datagram);

	ipc_send(session, datagram);
	free(datagram);

	return 0;

}
#endif

int ipc_send(ipc_session session, DB_DATAGRAM* data) {

	if (data->size > DATAGRAM_MAXSIZE) {
		return -1;
	}

#ifdef SERVER
	write(session->serverfd_w, data, data->size);
#else
	write(session->serverfd_r, data, data->size);
#endif

	return 0;

}

DB_DATAGRAM* ipc_receive(ipc_session session) {

	int size;
	DB_DATAGRAM* datagram = (DB_DATAGRAM*)malloc(DATAGRAM_MAXSIZE);

#ifdef SERVER
	size = read(session->serverfd_r, datagram, DATAGRAM_MAXSIZE);
#else
	size = read(session->serverfd_w, datagram, DATAGRAM_MAXSIZE);
#endif

	datagram->size = size;

	return datagram;

}

void ipc_disconnect(ipc_session session) {



}

void ipc_free(ipc_session session) {

	free(session);

}
