#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>


#include "config.h"
#include "database.h"
#include "ipc.h"

static int listenfd;

void ipc_listen(){

	struct sockaddr_in serv_addr; 

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SOCKET_PORT); 

	printf("Binding network interface to port %d\n", SOCKET_PORT);

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

	listen(listenfd, SOCKET_BACKLOG); 

}

void ipc_accept(){

	int pid;
	int clientfd;
	struct sockaddr_in cli_addr; 

	int size = sizeof(struct sockaddr_in);

	printf("Listening to connections...\n");

	clientfd = accept(listenfd, (struct sockaddr*)&cli_addr, (socklen_t*)&size); 

	switch (pid = fork()){
		case -1:
			printf("fork failed.\n");
			exit(1);
			break;

		case 0: /* hijo */

			printf("Fork creado, esperando datos.\n");

			int read_size;
			char client_message[2000];
			 
			//Receive a message from client
			while((read_size = recv(clientfd , client_message , 2000 , 0)) > 0 ){
				//end of string marker
				client_message[read_size] = '\0';
				
				printf("Mensaje recibido: %s\n", client_message);

				//Send the message back to client
				write(clientfd , client_message , strlen(client_message));
				
				//clear the message buffer
				memset(client_message, 0, 2000);
			}
			 
			if(read_size == 0){
				puts("Client disconnected");
				fflush(stdout);
			}else if(read_size == -1){
				perror("recv failed");
			}


			break;

		default: /* padre */

			break;
	}


}

// void ipc_connect(){

// }

// int ipc_send(DB_DATAGRAM* data){

// }

// DB_DATAGRAM* ipc_receive(){

// }

void ipc_disconnect(){

}

