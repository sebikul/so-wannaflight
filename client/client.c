#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "config.h"
#include "ipc.h"
#include "database.h"
#include "libclient.h"

static ipc_session session;
int is_admin = 0;

static void send_exit() {

	DB_DATAGRAM *datagram;

	datagram = (DB_DATAGRAM*) malloc(sizeof(DB_DATAGRAM));
	datagram->size = sizeof(DB_DATAGRAM);
	datagram->opcode = OP_EXIT;

	printf("Enviando comando EXIT\n");

	ipc_send(session, datagram);
	free(datagram);
}

void int_handler(int s) {
	printf("Limpiando antes de desconectar.\n");

	send_exit();

	ipc_disconnect(session);
	ipc_free(session);
	exit(0);
}

int main(int argc, char** argv) {

	char buffer[DATAGRAM_MAXSIZE] = {0};
	int n, err;

	system("clear");
	printf("Iniciando cliente...\n");

	signal(SIGINT, int_handler);

	session = ipc_newsession();
	err = ipc_connect(session, argc - 1, ++argv);

	if (err == -1) {
		fprintf(stderr, "Cantidad de argumentos invalida.\n");
		exit(1);
	}

	printf("----------------------------------\n");
	printf(COLOR_GREEN "Ejecute 'help' para obtener ayuda.\n" COLOR_RESET);
	print_prompt();

	while ((n = read(0, buffer, DATAGRAM_MAXSIZE)) > 0 ) {

		buffer[n - 1] = 0;

		if (strlen(buffer) == 0) {
			continue;
		}

		if (strcmp(buffer, "exit") == 0) {
			send_exit();
			break;
		} else {

			DB_DATAGRAM *datagram = command_to_datagram(buffer);

			if (datagram == NULL) {
				continue;
			}

			ipc_send(session, datagram);
			free(datagram);

			datagram = ipc_receive(session);
			parse_answer(datagram);
			free(datagram);

			print_prompt();
		}
	}

	ipc_disconnect(session);
	ipc_free(session);

	return 0;
}
