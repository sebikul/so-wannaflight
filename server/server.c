#include <stdio.h>
#include "ipc.h"

#define BUFFER_SIZE 50


int main(int argc, char** argv){
	
	// char buffer[BUFFER_SIZE];
	// char c;
	// int read=0;

	printf("Starting server...\n");


	ipc_listen();

		// while((c=getchar())!='\n' &&  read<BUFFER_SIZE){
		// 		buffer[read++]=c;
		// }

		//buffer tiene el comando a ejecutar


	

	
	return 0;
}