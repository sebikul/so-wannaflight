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

struct session_t{
	int serverfd;

	int otherfd;
	struct sockaddr_in cli_addr;

	char* client_ip;
};

ipc_session ipc_newsession(){

	ipc_session session = (ipc_session) malloc(sizeof(struct session_t));

	return (ipc_session)session;

}

#ifdef SERVER
int ipc_listen(ipc_session session, int argc, char** args) {

	int port;

	struct sockaddr_in serv_addr;

	if (argc != 1) {
		return -1;
	}

	port = atoi(args[0]);

	session->serverfd = socket(AF_INET, SOCK_STREAM, 0);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	SRVPRINT("Binding network interface to port %d\n", port);

	bind(session->serverfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	listen(session->serverfd, SOCKET_BACKLOG);

	return 0;

}

void ipc_accept(ipc_session session) {

	int size = sizeof(struct sockaddr_in);

	SRVPRINTE("Listening to connections...\n");

	session->otherfd = accept(session->serverfd, (struct sockaddr*)&session->cli_addr, (socklen_t*)&size);

}

void ipc_sync(ipc_session session) {

	session->client_ip = inet_ntoa(session->cli_addr.sin_addr);

	SRVPRINT("Cliente %s conectado, esperando datos.\n", session->client_ip);

}

void ipc_waitsync(ipc_session session) {

	//Desconectamos el cliente del proceso padre, solo el
	//hijo necesita el fd del socket nuevo.
	ipc_disconnect(session);

}
#endif

#ifdef CLIENT
int ipc_connect(ipc_session session, int argc, char** args) {

	struct sockaddr_in server;
	int port;

	if (argc != 2) {
		return -1;
	}

	//Create socket
	session->otherfd = socket(AF_INET , SOCK_STREAM , 0);
	if (session->otherfd == -1) {
		printf("Could not create socket");
	}

	port = atoi(args[1]);

	printf("Connecting to %s:%d\n", args[0], port);

	server.sin_addr.s_addr = inet_addr(args[0]);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	//Connect to remote server
	if (connect(session->otherfd , (struct sockaddr *)&server, sizeof(server)) < 0) {
		puts("connect error");
		return 1;
	}

	printf("Connected\n");

	printf("Listo para enviar comandos...\n");

	return 0;

}
#endif

int ipc_send(ipc_session session, DB_DATAGRAM* data) {

	//Send the message back to client
	write(session->otherfd , data , data->size);

	return 0;

}

DB_DATAGRAM* ipc_receive(ipc_session session) {

	int size;
	DB_DATAGRAM* datagram = (DB_DATAGRAM*)malloc(DATAGRAM_MAXSIZE);

	//Receive a message from client
	size = recv(session->otherfd , datagram , DATAGRAM_MAXSIZE , 0);

	if (size > 0) {

		datagram->size = size;

		return datagram;

	} else if (size == 0) {
		puts("Client disconnected");
		return NULL;
	} else {
		perror("recv failed");
		exit(1);
	}

}

void ipc_disconnect(ipc_session session) {

	close(session->otherfd);
	session->otherfd = 0;

}

void ipc_free(ipc_session session) {

	free(session);

}
