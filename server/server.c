#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "database.h"
#include "ipc.h"
#include "libserver.h"

int cli_count = 0;
static ipc_session session;

void int_handler(int s) {
	printf("Limpiando antes de salir!\n");
	ipc_disconnect(session);
	ipc_free(session);
	db_close();
	exit(0);
}

#ifdef FIFO
void sigpipe_handler(int s) {
	printf("Recibido SIGPIPE.\n");
	ipc_disconnect(session);
	ipc_free(session);
	db_close();
	exit(0);
}
#endif

void serve() {
	while (1) {
		DB_DATAGRAM* datagram, *ans;
		datagram = ipc_receive(session);
		if (datagram->opcode == OP_EXIT) {
			CLIPRINTE("Comando de salida recibido!\n");
			free(datagram);
			ipc_disconnect(session);
			//FIXME: Deberia liberar los recursos, pero no liberar el semaforo de cola
			//ipc_free(session);
			exit(0);
		}
		ans = execute_datagram(datagram);
		free(datagram);
		ipc_send(session, ans);
		free(ans);
	}
}

int main(int argc, char** argv) {
	int err;
	int pid = 0;
	system("clear");
	printf("Starting server...\n");
	signal(SIGINT, int_handler);
#ifdef FIFO
	signal(SIGPIPE, sigpipe_handler);
#endif
	printf("Opening database...\n");
	session = ipc_newsession();
	err = ipc_listen(session, argc - 1, ++argv);
	if (err == -1) {
		fprintf(stderr, "Invalid argument count.\n");
		exit(1);
	}
	while (1) {
		ipc_accept(session);
		cli_count++;
		if (ipc_shouldfork()) {
			pid = fork();
		}
		switch (pid) {
		case -1:
			printf("fork failed.\n");
			exit(1);
			break;
		case 0: /* hijo */
			ipc_sync(session);
			db_open(DB_PATH);
			serve();
			return 0;
			break;
		default:
			//En caso de ser necesario, esperamos que el cliente termine de
			//sincronizar con el servidor para aceptar el proximo cliente.
			ipc_waitsync(session);
			printf("Cliente aceptado. Esperando mas clientes...\n");
			break;
		}
	}
	return 0;
}
