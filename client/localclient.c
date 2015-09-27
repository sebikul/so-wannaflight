#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "config.h"
#include "database.h"
#include "libclient.h"
#include "libserver.h"
#include "filemutex.h"

int is_admin = 0;


void int_handler(int s) {
	printf("Limpiando antes de desconectar.\n");

	//clear_locks();

	exit(0);
}

int main(int argc, char* argv[]) {
	char buffer[DATAGRAM_MAXSIZE] = {0};
	int n;

	system("clear");
	printf("Iniciando cliente...\n");

	fmutex_t mutex = fmutex_new(LOCALCLIENT_MUTEX_PATH);

	fmutex_enter(mutex);

	printf("Archivo bloqueado. Presione enter para continuar.\n");
	getchar();

	fmutex_leave(mutex);
	fmutex_free(mutex);

	//lockdb();
	exit(0);

	db_open(DB_PATH);

	signal(SIGINT, int_handler);

	printf("----------------------------------\n");
	printf(COLOR_GREEN "Ejecute 'help' para obtener ayuda.\n" COLOR_RESET);
	print_prompt();

	while ((n = read(0, buffer, DATAGRAM_MAXSIZE)) > 0 ) {

		buffer[n - 1] = 0;

		if (strlen(buffer) == 0) {
			continue;
		}

		if (strcmp(buffer, "exit") == 0) {
			//TODO limpiar blocks
			//clear_locks();
			break;
		} else {

			DB_DATAGRAM* datagram = command_to_datagram(buffer);
			DB_DATAGRAM* ans;

			if (datagram == NULL) {
				continue;
			}

			ans = execute_datagram(datagram);
			free(datagram);

			parse_answer(ans);
			free(ans);

			print_prompt();
		}
	}


	return 0;
}
