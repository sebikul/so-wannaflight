#include "database.h"

void ipc_listen();

void ipc_connect();

int ipc_send(DB_DATAGRAM* data);

DB_DATAGRAM* ipc_receive();

void ipc_disconnect();
