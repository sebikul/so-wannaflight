#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "config.h"
#include "database.h"
#include "ipc.h"
#include "libserver.h"

int cli_count = 0;
static ipc_session session;

static void close_conn() {
	ipc_disconnect(session);
	ipc_free(session);
	db_close();
	exit(0);
}

void int_handler(int s) {

	if (s == SIGINT) {
		printf("Limpiando antes de salir!\n");

		close_conn();

	} else if (s == SIGCHLD) {
		int status;

		//Vamos a consumir los procesos hijos para que no queden zombies en el sistema
		printf("Proceso hijo terminado.");
		waitpid(-1, &status, 0);
		printf(" status: %d\n", status);
	}
}

#ifdef FIFO
void sigpipe_handler(int s) {
	printf("Recibido SIGPIPE.\n");

	close_conn();
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

		if(ans!=NULL){
			ipc_send(session, ans);
			free(ans);
		}
	}
}

int main(int argc, char** argv) {

	int err;
	int pid = 0;

	system("clear");
	printf("Starting server...\n");

	signal(SIGINT, int_handler);
	//signal(SIGCHLD, int_handler);

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
