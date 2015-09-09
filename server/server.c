#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "ipc.h"

#define BUFFER_SIZE 50

int cli_count = 0;

void int_handler(int s){
	printf("Cleaning up before exit!\n");
	ipc_disconnect();
	exit(0); 
}


int main(int argc, char** argv){
	
	int err;

	// char buffer[BUFFER_SIZE];
	// char c;
	// int read=0;

	printf("Starting server...\n");

	signal(SIGINT, int_handler);

	err = ipc_listen(argc-1, ++argv);

	if(err==-1){
		fprintf(stderr, "Invalid argument count.\n");
		exit(1);
	}

	while(1){
		ipc_accept();

		printf("Cliente aceptado. Esperando mas clientes...\n");

	}


	
	return 0;
}