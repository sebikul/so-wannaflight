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

static int socketfd;

#ifdef SERVER
extern int cli_count;
#endif

#ifdef SERVER
int ipc_listen(int argc, char** args){

	int port;

	struct sockaddr_in serv_addr; 

	if(argc != 1){
		return -1;
	}

	port = atoi(args[0]);

	socketfd = socket(AF_INET, SOCK_STREAM, 0);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port); 

	SRVPRINT("Binding network interface to port %d\n", port);

	bind(socketfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

	listen(socketfd, SOCKET_BACKLOG); 

}
#endif


#ifdef SERVER
void ipc_accept(){

	int pid;
	int clientfd;
	struct sockaddr_in cli_addr; 

	int size = sizeof(struct sockaddr_in);

	SRVPRINTE("Listening to connections...\n");

	clientfd = accept(socketfd, (struct sockaddr*)&cli_addr, (socklen_t*)&size); 

	cli_count++;

	switch (pid = fork()){
		case -1:
			SRVPRINTE("fork failed.\n");
			exit(1);
			break;

		case 0: /* hijo */

			SRVPRINTE("Fork creado, esperando datos.\n");

			int read_size;
			char client_message[2000];
			 
			//Receive a message from client
			while((read_size = recv(clientfd , client_message , 2000 , 0)) > 0 ){
				//end of string marker
				client_message[read_size] = '\0';
				
				CLIPRINT("Mensaje recibido: %s\n", client_message);

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

			close(clientfd);


			break;

		default: /* padre */

			close(clientfd);

			break;
	}


}
#endif

int ipc_connect(int argc, char** args){

	struct sockaddr_in server;
	int port;
	
	char buffer[SHMEM_SIZE] = {0};
	int n;

	if(argc != 2){
		return -1;
	}

	//Create socket
	socketfd = socket(AF_INET , SOCK_STREAM , 0);
	if (socketfd == -1){
		printf("Could not create socket");
	}

	port = atoi(args[1]);

	printf("Connecting to %s:%d\n", args[0], port);
		
	server.sin_addr.s_addr = inet_addr(args[0]);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	//Connect to remote server
	if (connect(socketfd , (struct sockaddr *)&server, sizeof(server)) < 0){
		puts("connect error");
		return 1;
	}
	
	printf("Connected\n");

	printf("Listo para enviar comandos...\n");	
	
	while ((n = read(0, buffer, SHMEM_SIZE)) > 0 ){

		if( send(socketfd , buffer , strlen(buffer) , 0) < 0){
			puts("Send failed");
			return 1;
		}
		
	}


	return 0;


}

// int ipc_send(DB_DATAGRAM* data){

// }

// DB_DATAGRAM* ipc_receive(){

// }

void ipc_disconnect(){

}

