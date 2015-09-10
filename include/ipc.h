#include "database.h"

#define CLIPRINTE(msg) 		printf("[CLIENT #%d] " msg, cli_count)
#define CLIPRINT(msg, ...) 	printf("[CLIENT #%d] " msg, cli_count, __VA_ARGS__)

#define SRVPRINTE(msg) 		printf("[SERVER] " msg)
#define SRVPRINT(msg, ...) 		printf("[SERVER] " msg, __VA_ARGS__)

int ipc_listen(int argc, char** args);

void ipc_accept();

int ipc_sync();

void ipc_waitsync();

int ipc_connect(int argc, char** args);

int ipc_send(DB_DATAGRAM* data);

DB_DATAGRAM* ipc_receive();

void ipc_disconnect();

void ipc_free();