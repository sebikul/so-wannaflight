#include "config.h"
#include "database.h"
#include "ipc.h"

struct session_t {

};


ipc_session ipc_newsession() {
  ipc_session session = (ipc_session) malloc(sizeof(struct session_t));
  return (ipc_session)session;
}


int ipc_listen(ipc_session session, int argc, char** args) {

}


void ipc_accept(ipc_session session) {

}


void ipc_sync(ipc_session session) {

}


void ipc_waitsync(ipc_session session) {

}


int ipc_connect(ipc_session session, int argc, char** args) {

}


int ipc_send(ipc_session session, DB_DATAGRAM* data) {

}


DB_DATAGRAM* ipc_receive(ipc_session session) {

}


void ipc_disconnect(ipc_session session) {

}


void ipc_free(ipc_session session) {

}
