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

#define DUMP_ARGS(argc, argv)	{\
									for (int i = 0; i < argc; i++) {\
										printf("%s ", argv[i]);\
									}\
									printf("\n");\
								}

#define FREE_ARGV(cmd)  {\
									for (int i = 0; i < cmd.argc; i++) {\
										free(cmd.argv[i]);\
									}\
									free(cmd.argv);\
								}

static ipc_session session;
static int is_admin = 0;

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

		*pos = 0;

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

static void parse_answer(DB_DATAGRAM* datagram) {


	switch (datagram->opcode) {

	case OP_PONG:
		printf("PONG: %s\n", datagram->dg_cmd);
		break;

	case OP_CONSULT:
		printf("Cantidad de vuelos encontrados: %d\n", datagram->dg_count);

		DUMP_DATAGRAM(datagram);

		// for(int i=0;i<datagram->dg_count;i++){

		// }
		break;



	}

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

		if (strlen(buffer) == 0) {
			continue;
		}

		if (strcmp(buffer, "exit") == 0) {
			send_exit();
			break;
		} else {

			DB_DATAGRAM *datagram;
			struct shell_cmd cmd = parse_command(buffer);

			if (strcmp(cmd.argv[0], "ping") == 0) {

				int size = sizeof(DB_DATAGRAM) + n;

				datagram = (DB_DATAGRAM*) malloc(size);
				datagram->size = size;
				datagram->opcode = OP_PING;

				strcpy(datagram->dg_cmd, buffer + 5);

			} else if (strcmp(cmd.argv[0], "consult") == 0) {

				datagram = (DB_DATAGRAM*) malloc(sizeof(DB_DATAGRAM));
				datagram->size = sizeof(DB_DATAGRAM);
				datagram->opcode = OP_CONSULT;

				datagram->dg_origin = -1;
				datagram->dg_destination = -1;

				int i = 1;

				printf("argc: %d\n", cmd.argc);

				while (i < cmd.argc) {

					printf("argc: %d, argv: <%s>\n", i, cmd.argv[i]);

					if (strcmp(cmd.argv[i], "to") == 0) {

						if (cmd.argc == i + 1) {
							printf("Comando invalido.\n");
						}

						i++;
						datagram->dg_destination = atoi(cmd.argv[i]);
						i++;
					} else if (strcmp(cmd.argv[i], "from") == 0) {

						if (cmd.argc == i + 1) {
							printf("Comando invalido.\n");
						}

						i++;
						datagram->dg_origin = atoi(cmd.argv[i]);
						i++;
					} else {
						printf("Comando invalido.\n");
					}

				}

				printf("Yendo de %d a %d\n", datagram->dg_origin, datagram->dg_destination);

			} else if (strcmp(cmd.argv[0], "purchase") == 0) {

				CHECK_ARGC(cmd.argc, 2);

				datagram = (DB_DATAGRAM*) malloc(sizeof(DB_DATAGRAM));
				datagram->size = sizeof(DB_DATAGRAM);
				datagram->opcode = OP_PURCHASE;

				datagram->dg_flightid = atoi(cmd.argv[1]);

			} else if (strcmp(cmd.argv[0], "cancel") == 0) {

				CHECK_ARGC(cmd.argc, 2);

				datagram = (DB_DATAGRAM*) malloc(sizeof(DB_DATAGRAM));
				datagram->size = sizeof(DB_DATAGRAM);
				datagram->opcode = OP_CANCEL;

				datagram->dg_seat = atoi(cmd.argv[1]);

			} else {

				if (strcmp(cmd.argv[0], "makeadmin") == 0) {
					is_admin = 1;
					printf("Privilegios de administrador activados!\n");
					FREE_ARGV(cmd);
					continue;
				}

				FREE_ARGV(cmd);

				printf("Comando invalido.\n");

				continue;

			}

			FREE_ARGV(cmd);

			ipc_send(session, datagram);
			free(datagram);

			datagram = ipc_receive(session);

			parse_answer(datagram);

		}

	}

	ipc_disconnect(session);
	ipc_free(session);

	return 0;
}
