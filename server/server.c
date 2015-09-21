#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "database.h"
#include "ipc.h"

#define BUFFER_SIZE 50

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
		//DUMP_DATAGRAM(datagram);

		if (datagram->opcode == OP_EXIT) {
			CLIPRINTE("Comando de salida recibido!\n");

			free(datagram);

			ipc_disconnect(session);

			//FIXME: Deberia liberar los recursos, pero no liberar el semaforo de cola
			//ipc_free(session);

			exit(0);
		}

		switch (datagram->opcode) {

		case OP_CONSULT:
			ans = consult_flights(datagram->dg_origin, datagram->dg_destination);
			break;

		case OP_PURCHASE:

			ans = malloc(sizeof(DB_DATAGRAM));
			ans->size = sizeof(DB_DATAGRAM);
			ans->opcode = OP_OK;
			ans->dg_seat = purchase(datagram->dg_flightid);
			break;

		case OP_CANCEL:

			ans = malloc(sizeof(DB_DATAGRAM));
			ans->size = sizeof(DB_DATAGRAM);
			ans->opcode = OP_OK;
			break;

		case OP_PING: {

			int size = sizeof(DB_DATAGRAM) + strlen(datagram->dg_cmd) + 1;

			ans = malloc(size);
			ans->size = size;
			ans->opcode = OP_PONG;
			strcpy(ans->dg_cmd, datagram->dg_cmd);
			break;
		}

		default:
			free(datagram);

			printf("Comando invalido.\n");

			continue;
		}

		free(datagram);

		//DUMP_DATAGRAM(datagram);
		ipc_send(session, ans);

		free(ans);
	}
}

int main(int argc, char** argv) {
	int err;
	int pid;

	system("clear");
	printf("Starting server...\n");

	signal(SIGINT, int_handler);

#ifdef FIFO
	signal(SIGPIPE, sigpipe_handler);
#endif

	printf("Opening database...\n");
	db_open(DB_PATH);

	session = ipc_newsession();
	err = ipc_listen(session, argc - 1, ++argv);

	if (err == -1) {
		fprintf(stderr, "Invalid argument count.\n");
		exit(1);
	}

	while (1) {

		ipc_accept(session);
		cli_count++;

		switch (pid = fork()) {
		case -1:
			printf("fork failed.\n");
			exit(1);
			break;

		case 0: /* hijo */
			ipc_sync(session);
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
