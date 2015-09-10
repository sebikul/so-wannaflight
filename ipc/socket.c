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


#ifdef SERVER
static int srvsocketfd;
extern int cli_count;
#endif

typedef struct {

	int clientfd;
	struct sockaddr_in cli_addr; 

} SOCK_CLIENT;

static SOCK_CLIENT sock_client;

#ifdef SERVER
int ipc_listen(int argc, char** args){

	int port;

	struct sockaddr_in serv_addr; 

	if(argc != 1){
		return -1;
	}

	port = atoi(args[0]);

	srvsocketfd = socket(AF_INET, SOCK_STREAM, 0);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port); 

	SRVPRINT("Binding network interface to port %d\n", port);

	bind(srvsocketfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

	listen(srvsocketfd, SOCKET_BACKLOG); 

	return 0;

}

void ipc_accept(){

	int size = sizeof(struct sockaddr_in);

	SRVPRINTE("Listening to connections...\n");

	sock_client.clientfd = accept(srvsocketfd, (struct sockaddr*)&sock_client.cli_addr, (socklen_t*)&size); 

}

void ipc_sync(){

	char *client_ip = inet_ntoa(sock_client.cli_addr.sin_addr);

	SRVPRINT("Cliente %s conectado, esperando datos.\n", client_ip);

}

void ipc_waitsync(){

	//Desconectamos el cliente del proceso padre, solo el
	//hijo necesita el fd del socket nuevo.
	ipc_disconnect();

}

#endif

#ifdef CLIENT
int ipc_connect(int argc, char** args){

	struct sockaddr_in server;
	int port;

	if(argc != 2){
		return -1;
	}

	//Create socket
	sock_client.clientfd = socket(AF_INET , SOCK_STREAM , 0);
	if (sock_client.clientfd == -1){
		printf("Could not create socket");
	}

	port = atoi(args[1]);

	printf("Connecting to %s:%d\n", args[0], port);
		
	server.sin_addr.s_addr = inet_addr(args[0]);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	//Connect to remote server
	if (connect(sock_client.clientfd , (struct sockaddr *)&server, sizeof(server)) < 0){
		puts("connect error");
		return 1;
	}
	
	printf("Connected\n");

	printf("Listo para enviar comandos...\n");	
	
	return 0;

}
#endif


int ipc_send(DB_DATAGRAM* data){

	//Send the message back to client
	write(sock_client.clientfd , data , data->size);

	return 0;

}

DB_DATAGRAM* ipc_receive(){

	int size;
	DB_DATAGRAM* datagram = (DB_DATAGRAM*)malloc(DATAGRAM_MAXSIZE);

	//Receive a message from client
	size = recv(sock_client.clientfd , datagram , DATAGRAM_MAXSIZE , 0);
	
	if(size > 0){

		datagram->size = size;

		return datagram;

	}else if(size == 0){
		puts("Client disconnected");
		return NULL;
	}else{
		perror("recv failed");
		exit(1);
	}

}

void ipc_disconnect(){

	close(sock_client.clientfd);
	sock_client.clientfd = 0;

}

void ipc_free(){

}
