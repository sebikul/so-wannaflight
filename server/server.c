#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "ipc.h"

#define BUFFER_SIZE 50

int cli_count = 0;

void int_handler(int s){
	printf("Cleaning up before exit!\n");
	ipc_disconnect();
	exit(0); 
}


void serve(int cli_count){

	static char* mensaje = "Mensaje recibido";
	int n = strlen(mensaje);

	while(1){

		DB_DATAGRAM* dg = ipc_receive();
		//DUMP_DATAGRAM(dg);

		CLIPRINT("Mensaje recibido: %s\n", dg->dg_cmd);

		// if(strcmp(dg->dg_cmd, "exit") == 0){
		// 	CLIPRINTE("Exit command received!\n");
		// 	dg->opcode = OP_EXIT;

		// 	DUMP_DATAGRAM(dg);
		// 	ipc_send(dg);

		// 	free(dg);
		// 	break;
		// }

		dg->size = sizeof(DB_DATAGRAM) + n;
		memcpy(dg->dg_cmd, mensaje, n);

		CLIPRINT("Enviando respuesta: %s\n", dg->dg_cmd);

		//DUMP_DATAGRAM(dg);
		ipc_send(dg);

		free(dg);

	}

	CLIPRINTE("Cliente desconectado. Limpiando...\n");

	ipc_disconnect();

	exit(0);

}

int main(int argc, char** argv){
	
	int err;
	int pid;

	printf("Starting server...\n");

	signal(SIGINT, int_handler);

	err = ipc_listen(argc-1, ++argv);

	if(err==-1){
		fprintf(stderr, "Invalid argument count.\n");
		exit(1);
	}

	while(1){

		ipc_accept();

		switch (pid = fork()){

			case -1:
				printf("fork failed.\n");
				exit(1);
				break;

			case 0:{ /* hijo */

				int cli_count;

				cli_count = ipc_sync();

				serve(cli_count);

				break;

			}

			default:
	 			
	 			//En caso de ser necesario, esperamos que el cliente termine de 
	 			//sincronizar con el servidor para aceptar el proximo cliente.
				ipc_waitsync();

				printf("Cliente aceptado. Esperando mas clientes...\n");

				break;

		}

	}

	return 0;
}