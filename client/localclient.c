#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "config.h"
#include "database.h"
#include "libclient.h"
#include "libserver.h"
#include "filemutex.h"

int is_admin = 0;
static fmutex_t mutex;
static int delay = 0;

void int_handler(int s) {

	fmutex_free(mutex);

	exit(0);
}

int main(int argc, char* argv[]) {
	char buffer[DATAGRAM_MAXSIZE] = {0};
	int n;

	system("clear");
	printf("Iniciando cliente...\n");

	mutex = fmutex_new(LOCALCLIENT_MUTEX_PATH);

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
			break;
		} else if (strncmp(buffer, "setdelay", 8) == 0) {

			struct shell_cmd cmd = parse_command(buffer);

			if (cmd.argc != 2) {
				printf("Comando invalido.\n");
			}

			delay = atoi(cmd.argv[1]);
			printf("Retardo seteado en %d segundos\n", delay);
			
			FREE_ARGV(cmd);
			print_prompt();

		} else {

			DB_DATAGRAM* datagram = command_to_datagram(buffer);
			DB_DATAGRAM* ans;

			if (datagram == NULL) {
				print_prompt();
				continue;
			}

			fmutex_enter(mutex);
			ans = execute_datagram(datagram);

			if (delay > 0) {
				sleep(delay);
			}

			fmutex_leave(mutex);

			free(datagram);
			parse_answer(ans);
			free(ans);

			print_prompt();
		}
	}

	fmutex_free(mutex);

	return 0;
}
