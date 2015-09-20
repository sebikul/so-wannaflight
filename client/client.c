#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "ipc.h"
#include "database.h"

#define BUFFER_SIZE 50
#define LEFT_STRIP(str)			while (*(++str) == ' ')
#define MAX_ARGS 				10
#define CMD_BUFFER_SIZE 		2*MAX_ARGS

#define CHECK_ARGC(argc, expected) 	if (argc != expected) {\
										printf("Cantidad de argumentos invalida.\n");\
										continue;\
									}

static ipc_session session;

struct shell_cmd {
	int argc;
	char** argv;
};

struct shell_cmd parse_command(char* command) {

	int argc = 0;
	char** argv = malloc(MAX_ARGS * sizeof(char*));
	struct shell_cmd ans;

	memset(&ans, 0, sizeof(struct shell_cmd));

	//Vamos a sacarle todos los espacion al principio del comando
	if (*command == ' ') {
		LEFT_STRIP(command);
	}

	while (*command != 0) {

		//alocamos espacio para el argumento que estamos parseando
		argv[argc] = malloc(CMD_BUFFER_SIZE * sizeof(char));


		//copiamos el puntero a la cadena por comodidad, para poder modificarlo
		char* pos = argv[argc];

		bool comillas = (*command == '"');

		if (comillas)
			command++;

		while (((!comillas && *command != ' ') || (comillas && *command != '"')) && *command != 0) {

			*pos = *command;
			pos++;

			command++;
		}

		if (comillas && *command == '"') {
			command++;
			comillas = 0;
		}

		if (comillas) {
			printf("Comando mal formateado. Contiene comillas sin cerrar!\n");
			return ans;
		}

		//si al argumento le siguen espacios los limpiamos
		if (*command == ' ') {
			LEFT_STRIP(command);
		}

		argc++;

	}

	ans.argc = argc;
	ans.argv = argv;

	return ans;

}

static void parse_answer(DB_DATAGRAM* datagram){

}

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

	printf("-----------------------------------------------------------------\n");
	printf("Los comandos disponibles son: consultar, comprar, cancelar, salir\n");

	while ((n = read(0, buffer, DATAGRAM_MAXSIZE)) > 0 ) {

		buffer[n - 1] = 0;

		if (strcmp(buffer, "exit") == 0) {
			send_exit();
			break;
		} else {

			DB_DATAGRAM *datagram;
			struct shell_cmd cmd = parse_command(buffer);

			if (strcmp(cmd.argv[0], "ping") == 0) {

				CHECK_ARGC(argc, 1);

				int size = sizeof(DB_DATAGRAM) + n;

				datagram = (DB_DATAGRAM*) malloc(size);
				datagram->size = size;
				datagram->opcode = OP_PING;

				strcpy(datagram->dg_cmd, buffer[5]);

			} else if (strcmp(cmd.argv[0], "consult") == 0) {

				CHECK_ARGC(argc, 3);

				datagram = (DB_DATAGRAM*) malloc(sizeof(DB_DATAGRAM));
				datagram->size = sizeof(DB_DATAGRAM);
				datagram->opcode = OP_CONSULT;

				datagram->dg_origin = atoi(argv[1]);
				datagram->dg_destination = atoi(argv[2]);


			} else if (strcmp(cmd.argv[0], "purchase") == 0) {

				CHECK_ARGC(argc, 2);

				datagram = (DB_DATAGRAM*) malloc(sizeof(DB_DATAGRAM));
				datagram->size = sizeof(DB_DATAGRAM);
				datagram->opcode = OP_PURCHASE;

				datagram->dg_flightid = atoi(argv[1]);

			}else if (strcmp(cmd.argv[0], "cancel") == 0) {

				CHECK_ARGC(argc, 2);

				datagram = (DB_DATAGRAM*) malloc(sizeof(DB_DATAGRAM));
				datagram->size = sizeof(DB_DATAGRAM);
				datagram->opcode = OP_CANCEL;

				datagram->dg_seat = atoi(argv[1]);

			}else{

			}

			for (int i = 0; i < cmd.argc; i++) {
				free(cmd.argv[i]);
			}
			free(cmd.argv);

			ipc_send(session, datagram);
			free(datagram);

			datagram = ipc_receive(session);

			parse_answer(datagram);

			DUMP_DATAGRAM(datagram);

		}

	}

	ipc_disconnect(session);
	ipc_free(session);

	return 0;
}
