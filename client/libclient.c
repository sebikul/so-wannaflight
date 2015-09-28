#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "database.h"
#include "libclient.h"

extern int is_admin;

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

void parse_answer(DB_DATAGRAM* datagram) {

	printf(COLOR_GREEN);

	switch (datagram->opcode) {

	case OP_PONG:
		printf("PONG: %s\n", datagram->dg_cmd);
		break;

	case OP_CONSULT:
		printf("Cantidad de vuelos encontrados: %d\n", datagram->dg_count);

		for (int i = 0; i < datagram->dg_count; i++) {

			DB_ENTRY entry = datagram->dg_results[i];
			char timebuf[32];

			strftime(timebuf, 32, "%d-%m-%Y %H:%M:%S", localtime(&entry.departure));

			printf("Encontrado vuelo con ID #%.4d de %d a %d, con fecha de salida %s\n", entry.id, entry.origin, entry.destination, timebuf);

		}

		break;

	case OP_PURCHASE:
		if (datagram->dg_seat == -1) {
			printf("El vuelo ingresado no existe.\n");
		} else {
			printf("Compra exitosa. ID del ticket: %.4d\n", datagram->dg_seat);
		}
		break;

	case OP_CANCEL:
		if (datagram->dg_result == TRUE) {
			printf("Compra cancelada de forma exitosa.\n");
		}
		break;

	case OP_ADDFLIGHT:
		printf("Agregado el vuelo con ID #%.4d\n", datagram->dg_flightid);

		break;

	default:
		printf("Datagrama desconocido:\n");

		DUMP_DATAGRAM(datagram);

	}

	printf(COLOR_RESET "\n");

}

DB_DATAGRAM* command_to_datagram(char* command) {

	DB_DATAGRAM* datagram;
	struct shell_cmd cmd = parse_command(command);

	//DUMP_ARGS(cmd.argc, cmd.argv);

	if (strcmp(cmd.argv[0], "ping") == 0) {

		int size = sizeof(DB_DATAGRAM) + strlen(command);

		datagram = (DB_DATAGRAM*) malloc(size);
		datagram->size = size;
		datagram->opcode = OP_PING;

		strcpy(datagram->dg_cmd, command + 5);

	} else if (strcmp(cmd.argv[0], "consult") == 0) {

		datagram = (DB_DATAGRAM*) malloc(sizeof(DB_DATAGRAM));
		datagram->size = sizeof(DB_DATAGRAM);
		datagram->opcode = OP_CONSULT;

		datagram->dg_origin = -1;
		datagram->dg_destination = -1;

		int i = 1;
		int had_error = 0;

		//printf("argc: %d\n", cmd.argc);

		while (i < cmd.argc) {

			//printf("argc: %d, argv: <%s>\n", i, cmd.argv[i]);

			if (strcmp(cmd.argv[i], "to") == 0) {

				if (cmd.argc == i + 1) {
					printf("Comando invalido.\n");
					had_error = 1;
					break;
				}

				i++;
				datagram->dg_destination = atoi(cmd.argv[i]);
				i++;
			} else if (strcmp(cmd.argv[i], "from") == 0) {

				if (cmd.argc == i + 1) {
					printf("Comando invalido.\n");
					had_error = 1;
					break;
				}

				i++;
				datagram->dg_origin = atoi(cmd.argv[i]);
				i++;
			} else {
				printf("Comando invalido.\n");
				had_error = 1;
				break;
			}
		}

		if (had_error) {
			return NULL;
		}

		//printf("Yendo de %d a %d\n", datagram->dg_origin, datagram->dg_destination);

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

	} else if (strcmp(cmd.argv[0], "help") == 0) {


		printf("Comandos disponibles:\n");
		printf("\t ping [...]: \tEnvia una cadena de texto al servidor, y éste responde con la misma cadena.\n");
		printf("\t consult [from <origen>] [to <destino>]: \tConsulta la disponibilidad de vuelos. El orden de los argumentos es intercambiable. Sin argumentos imprime toda la lista de vuelos.\n");
		printf("\t purchase <flightid>: \tCompra un pasaje para el vuelo ingresado.\n");
		printf("\t cancel <purchaseid>: \tCancela el pasaje ingresado.\n");
		printf("\t exit: \tSale del cliente.\n");

		printf("\t makeadmin: \tDa privilegios de escritura al usuario.\n");
		printf("\t addflight <origen> <destino> \"dd-mm-YYYY HH:MM:SS\": \tAgrega un vuelo a la base de datos\n");

#ifdef LOCALCLIENT
		printf("\t setdelay <delay>: \tSetea un delay arbitrario para poder probar las funciones de exclusion mutua.\n");
#endif

		return NULL;

	} else  if (is_admin && strcmp(cmd.argv[0], "addflight") == 0) {

		struct tm departure;
		char timebuf[32];

		CHECK_ARGC(cmd.argc, 4);

		datagram = (DB_DATAGRAM*) malloc(sizeof(DB_DATAGRAM));
		datagram->size = sizeof(DB_DATAGRAM);
		datagram->opcode = OP_ADDFLIGHT;

		datagram->dg_results[0].origin = atoi(cmd.argv[1]);
		datagram->dg_results[0].destination = atoi(cmd.argv[2]);

		if (strptime(cmd.argv[3], "%d-%m-%Y %H:%M:%S", &departure) == NULL) {
			printf("Comando invalido.\n");
			FREE_ARGV(cmd);
			return NULL;
		}

		datagram->dg_results[0].departure = mktime(&departure);

		strftime(timebuf, 32, "%d-%m-%Y %H:%M:%S", localtime(&datagram->dg_results[0].departure));
		printf("Agregando vuelo de %d a %d con fecha %s\n", datagram->dg_results[0].origin, datagram->dg_results[0].destination, timebuf);

	} else {

		if (strcmp(cmd.argv[0], "makeadmin") == 0) {
			is_admin = 1;
			printf(COLOR_GREEN "Privilegios de administrador activados!\n\n" COLOR_RESET);
			FREE_ARGV(cmd);
			printf("$ > ");
			fflush(stdout);
			return NULL;
		}

		FREE_ARGV(cmd);

		printf("Comando invalido -1.\n");

		return NULL;
	}

	FREE_ARGV(cmd);

	return datagram;
}

void print_prompt() {
	printf("$ > ");
	fflush(stdout);
}
