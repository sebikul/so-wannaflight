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
	exit(0);
}

#ifdef FIFO
void sigpipe_handler(int s) {
	printf("Recibido SIGPIPE.\n");
	ipc_disconnect(session);
	ipc_free(session);
	exit(0);
}
#endif

void serve() {
	static char* mensaje = "Mensaje recibido";

	int n = strlen(mensaje);

	while (1) {

		DB_DATAGRAM* dg = ipc_receive(session);
		//DUMP_DATAGRAM(dg);
		CLIPRINT("Mensaje recibido: %s\n", dg->dg_cmd);

		if (strcmp(dg->dg_cmd, "salir") == 0) {
			CLIPRINTE("Comando de salida recibido!\n");
			dg->opcode = OP_EXIT;
			ipc_send(session, dg);
			free(dg);
			CLIPRINTE("Cliente desconectado. Limpiando...\n");
			ipc_disconnect(session);
			ipc_free(session);
			exit(0);
		}

		if (strcmp(datagram->dg_cmd, "consultar") == 0) {
		}
		if (strcmp(datagram->dg_cmd, "comprar") == 0) {
		}
		if (strcmp(datagram->dg_cmd, "cancelar") == 0) {
		}

		free(datagram);

		datagram = malloc(sizeof(DB_DATAGRAM) + n);
		datagram->size = sizeof(DB_DATAGRAM) + n;
		datagram->opcode = OP_CMD;

		strcpy(datagram->dg_cmd, mensaje);

		CLIPRINT("Enviando respuesta: %s\n", datagram->dg_cmd);
		//DUMP_DATAGRAM(datagram);
		ipc_send(session, datagram);
		free(datagram);
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

	// consult_flights(20, 30);
	// exit(0);

	// //////////////////////////////////////////////

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
