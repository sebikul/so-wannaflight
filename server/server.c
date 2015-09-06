#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "ipc.h"

#define BUFFER_SIZE 50


void int_handler(int s){
	printf("Cleaning up before exit!\n");
	ipc_disconnect();
	exit(0); 
}


int main(int argc, char** argv){
	
	// char buffer[BUFFER_SIZE];
	// char c;
	// int read=0;

	printf("Starting server...\n");


	signal(SIGINT, int_handler);

	ipc_listen();

		// while((c=getchar())!='\n' &&  read<BUFFER_SIZE){
		// 		buffer[read++]=c;
		// }

		//buffer tiene el comando a ejecutar


	

	
	return 0;
}