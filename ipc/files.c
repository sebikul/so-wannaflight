#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "config.h"
#include "database.h"
#include "ipc.h"
#include "semaphore.h"

struct session_t {
	char* path;

	int serverfd;
	int otherpid;
//	fmutex_t mutex;

	int queueid;
};

#ifdef SERVER
extern int cli_count;
#endif

void sigusr_handler(int s) {

	printf("Señal SIGUSR1 recibida! Soy %d\n", getpid());

	return;
}

static void build_path(const char* path, char** dest) {

	*dest = malloc(strlen(path) + 1);
	strcpy(*dest, path);

}

static int file_exist(const char* path) {
	return (access(path, 0) == 0);
}

ipc_session ipc_newsession() {

	ipc_session session = (ipc_session) malloc(sizeof(struct session_t));

	return (ipc_session)session;
}

#ifdef SERVER
int ipc_listen(ipc_session session, int argc, char** args) {

	sem_queue_init(&session->queueid, 1);

	//Inicializamos el valor del semaforo de la cola.
	sem_set(session->queueid, SEM_QUEUE, 0);

	//session->mutex = fmutex_new(FILES_INITIAL_PATH);

	signal(SIGUSR1, sigusr_handler);

	if ( file_exist(FILES_INITIAL_PATH)) {
		fprintf(stderr, "Error al crear el archivo en " FILES_INITIAL_PATH ", ya existe.\n");
		return -1;
	}

	build_path(FILES_INITIAL_PATH, &session->path);

	printf("Abriendo archivo publico" FILES_INITIAL_PATH "\n");

	session->serverfd = open(FILES_INITIAL_PATH, O_RDWR | O_CREAT);
	fchmod(session->serverfd, S_IWUSR | S_IRUSR);

	if (session->serverfd == -1) {
		printf("errno_r: %d\n", errno);
		return -1;
	}

	return 0;
}

void ipc_accept(ipc_session session) {

	DB_DATAGRAM* datagram;
	char pid[10];

	printf("Esperando cliente...\n");

	sprintf(pid, "%d", getpid());
	printf("Escribiendo pid del servidor: %s\n", pid);
	lseek(session->serverfd, 0, SEEK_SET);
	write(session->serverfd, pid, strlen(pid));

	//Bloquea hasta que recibimos una señal del cliente
	//fmutex_enter(session->mutex);

	//STEP-1
	datagram = ipc_receive(session);

	if (datagram->opcode != OP_CONNECT) {
		CLIPRINTE("Error en el protocolo de sincronizacion.\n");
		DUMP_DATAGRAM(datagram);
		ipc_disconnect(session);
		ipc_free(session);
		exit(1);
	}

	//Seteamos el pid del cliente para poder ejecutar el send.
	session->otherpid = datagram->dg_pid;
	printf("pid del cliente: %d\n", session->otherpid);

	free(datagram);

	printf("Cliente conectado...\n");
}

void ipc_sync(ipc_session session) {

	char newpath[64];
	DB_DATAGRAM* datagram;
	int size;
	int newfd;
	char pid[10];

	sprintf(newpath, "%s-%d", FILES_INITIAL_PATH, cli_count);
	printf("Creando nuevo archivo de comunicacion: %s\n", newpath);
	newfd = open(newpath, O_RDWR | O_CREAT);
	fchmod(newfd, S_IWUSR | S_IRUSR);

	if (newfd == -1) {
		fprintf(stderr, "Error al crear el archivo en %s\n", newpath);
		exit(1);
	}

	// sprintf(pid, "%d", getpid());
	// printf("Escribiendo nuevo pid del servidor: %s\n", pid);
	// write(session->serverfd, pid, strlen(pid));

	//A esta altura los archivos existen en el fs, pero nadie esta conectado. Falta mandarselos
	//al cliente, y luego reemplazar los actuales para poder aceptar al proximo cliente.

	//Creamos el prefijo del path para pasarselo al cliente y que lo use
	//para conectarse a los nuevos FIFO.

	size = sizeof(DB_DATAGRAM) + strlen(newpath) + 1;

	datagram = malloc(size);

	strcpy(datagram->dg_cmd, newpath);
	datagram->size = size;
	datagram->dg_pid = getpid();

	DUMP_DATAGRAM(datagram);

	//Enviamos el nuevo path del archivo
	ipc_send(session, datagram);
	free(datagram);

	close(session->serverfd);
	session->serverfd = newfd;
	free(session->path);
	build_path(newpath, &session->path);


	//fmutex_leave(session->mutex);
	sem_up(session->queueid, SEM_QUEUE);

	printf("Esperando ACK\n");

	//Bloquea hasta que reciba la señal del cliente
	datagram = ipc_receive(session);

	if (datagram->opcode != OP_CONNECT) {
		CLIPRINTE("Error en el protocolo de sincronizacion 2.\n");
		DUMP_DATAGRAM(datagram);
		ipc_disconnect(session);
		ipc_free(session);
		exit(1);
	}

	printf("Sincronizacion realizada!\n");

	free(datagram);
}

void ipc_waitsync(ipc_session session) {

	sem_down(session->queueid, SEM_QUEUE);

}
#endif

#ifdef CLIENT
int ipc_connect(ipc_session session, int argc, char** args) {

	char pid[10];

	DB_DATAGRAM* datagram;

	if ( !file_exist(FILES_INITIAL_PATH)) {
		fprintf(stderr, "Error al conectarse con el archivo publico " FILES_INITIAL_PATH "\n");
		exit(1);
	}

	signal(SIGUSR1, sigusr_handler);

	session->serverfd = open(FILES_INITIAL_PATH, O_RDWR);
	build_path(FILES_INITIAL_PATH, &session->path);

	if (session->serverfd == -1) {
		printf("errno_r: %d\n", errno);
		return -1;
	}

	read(session->serverfd, pid, 10);
	printf("Guardado pid del servidor: %s\n", pid);
	session->otherpid = atoi(pid);

	printf("Escribiendo pedido de conexion...\n");

	datagram = (DB_DATAGRAM*) malloc(sizeof(DB_DATAGRAM));
	datagram->opcode = OP_CONNECT;
	datagram->size = sizeof(DB_DATAGRAM);
	datagram->dg_pid = getpid();

	//DUMP_DATAGRAM(datagram);

	//STEP-1
	ipc_send(session, datagram);
	free(datagram);
	//El servidor se desbloquea

	printf("Esperando nuevo archivo de comunicacion...\n");

	datagram = ipc_receive(session);

	printf("Nuevo archivo a usar: %s\n", datagram->dg_cmd);
	printf("Nuevo pid del servidor: %d\n", datagram->dg_pid);
	session->otherpid = datagram->dg_pid;

	close(session->serverfd);

	free(session->path);
	build_path(datagram->dg_cmd, &session->path);
	printf("Abriendo nuevo archivo de comunicacion: %s\n", session->path);
	session->serverfd = open(session->path, O_RDWR);


	// read(session->serverfd, pid, 10);
	// printf("Guardado nuevo pid del servidor: %s\n", pid);
	// session->otherpid = atoi(pid);

	printf("Enviando ACK\n");
	datagram = realloc(datagram, sizeof(DB_DATAGRAM));

	memset(datagram, 0, sizeof(DB_DATAGRAM));
	datagram->size = sizeof(DB_DATAGRAM);
	datagram->opcode = OP_CONNECT;

	ipc_send(session, datagram);
	free(datagram);

	return 0;
}
#endif

int ipc_send(ipc_session session, DB_DATAGRAM* data) {

	if (data->size > DATAGRAM_MAXSIZE) {
		return -1;
	}

	lseek(session->serverfd, 0, SEEK_SET);
	write(session->serverfd, data, data->size);

	//printf("Escribiendo datagrama en %s\n", session->path);
	//Mandamos la señal para que se levante la otra parte.

	//printf("Enviando señal al pid: %d\n", session->otherpid);
	kill(session->otherpid, SIGUSR1);

	return 0;
}

DB_DATAGRAM* ipc_receive(ipc_session session) {

	int size;
	sigset_t mask;

	DB_DATAGRAM* datagram = (DB_DATAGRAM*)malloc(DATAGRAM_MAXSIZE);

	sigfillset(&mask);
	sigdelset(&mask, SIGUSR1);
	sigdelset(&mask, SIGINT);
	sigsuspend(&mask);

	lseek(session->serverfd, 0, SEEK_SET);
	size = read(session->serverfd, datagram, DATAGRAM_MAXSIZE);

	//printf("Leyendo datagrama de %s\n", session->path);

	if (size == -1) {
		printf("Read error: %d\n", errno);
		free(datagram);
		return NULL;
	}

	datagram->size = size;

	datagram = realloc(datagram, datagram->size);

	return datagram;
}

void ipc_disconnect(ipc_session session) {

	close(session->serverfd);

#ifdef SERVER
	//unlink(session->path);
#endif
}

void ipc_free(ipc_session session) {

#ifdef SERVER
	sem_queue_destroy(session->queueid);
#endif

	free(session->path);

	free(session);
}

bool ipc_shouldfork() {
	return TRUE;
}
