#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "ipc.h"
#include "database.h"

#define BUFFER_SIZE 50

void int_handler(int s){
	printf("Cleaning up before exit!\n");
	ipc_disconnect();
	exit(0); 
}

int main(int argc, char** argv){

	char buffer[SHMEM_SIZE] = {0};
	int n, size, err;
	DB_DATAGRAM *datagram;

	printf("Starting client...\n");

	signal(SIGINT, int_handler);

	err = ipc_connect(argc-1, ++argv);

	if(err == -1){
		fprintf(stderr, "Invalid argument count.\n");
		exit(1);
	}

	while ((n = read(0, buffer, SHMEM_SIZE)) > 0 ){

		buffer[n-1]=0;
		size = sizeof(DB_DATAGRAM) + n;

		datagram = (DB_DATAGRAM*) calloc(1, size);

		datagram->size = size;
		datagram->opcode=OP_CMD;
		strcpy(datagram->dg_cmd, buffer);
		datagram->dg_cmd[n] = 0;

		printf("Enviando comando: %s\n", datagram->dg_cmd);

		//STEP-4 Up -> Client
		ipc_send(datagram);

		free(datagram);

		datagram = ipc_receive();

		printf("Respuesta: %s\n", datagram->dg_cmd);


	}

	printf("Disconnecting\n");
	ipc_disconnect();


}