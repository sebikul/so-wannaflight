#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "config.h"
#include "ipc.h"
#include "database.h"

#define BUFFER_SIZE 50

static ipc_session session;

void int_handler(int s) {
	printf("Cleaning up before exit!\n");
	ipc_disconnect(session);
	exit(0);
}

static void send_cmd(char* cmd, int n) {

	DB_DATAGRAM *datagram;

	int size = sizeof(DB_DATAGRAM) + n;

	datagram = (DB_DATAGRAM*) calloc(1, size);

	datagram->size = size;
	datagram->opcode = OP_CMD;
	strcpy(datagram->dg_cmd, cmd);

	printf("Enviando comando: %s\n", datagram->dg_cmd);

	ipc_send(session, datagram);

	free(datagram);
}

int main(int argc, char** argv) {

	char buffer[SHMEM_SIZE] = {0};
	int n, err;
	DB_DATAGRAM *datagram;

	printf("Starting client...\n");

	//signal(SIGINT, int_handler);

	session = ipc_newsession();

	err = ipc_connect(session, argc - 1, ++argv);

	if (err == -1) {
		fprintf(stderr, "Invalid argument count.\n");
		exit(1);
	}

	while ((n = read(0, buffer, SHMEM_SIZE)) > 0 ) {

		buffer[n - 1] = 0;

		send_cmd(buffer, n);

		datagram = ipc_receive(session);

		printf("Respuesta: %s\n", datagram->dg_cmd);

		if (datagram->opcode == OP_EXIT) {
			printf("Exiting...\n");
			break;
		}

	}

	printf("Disconnecting\n");
	ipc_disconnect(session);

	ipc_free(session);

	return 0;

}
