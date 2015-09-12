#include "database.h"

#define CLIPRINTE(msg) 		printf("[CLIENT #%d] " msg, cli_count)
#define CLIPRINT(msg, ...) 	printf("[CLIENT #%d] " msg, cli_count, __VA_ARGS__)

#define SRVPRINTE(msg) 		printf("[SERVER] " msg)
#define SRVPRINT(msg, ...) 	printf("[SERVER] " msg, __VA_ARGS__)


/**
 * Método que llama el servidor para preparar el mecanismo de IPC.
 * @param  argc Cantidad de argumentos que recibe
 * @param  args Vector de argumentos
 * @return      0 si no hubo error, -1 si la cantidad de argumentos es incorrecta.
 */
int ipc_listen(int argc, char** args);

/**
 * Método bloqueante que espera que se conecte un cliente al servidor.
 */
void ipc_accept();

/**
 * Método que llama el servidor antes de comenzar a servir contenido.
 * Permite sincronizar cliente-servidor en caso de que sea necesario.
 */
void ipc_sync();

/**
 * Método que permite esperar a que el cliente se haya sincronizado con el servidor antes
 * de aceptar el próximo cliente.
 */
void ipc_waitsync();

/**
 * Método que llama el cliente para establecer una conexión con un servidor.
 * @param  argc Cantidad de argumentos que recibe
 * @param  args Vector de argumentos
 * @return      0 si no hubo error, -1 si la cantidad de argumentos es incorrecta.
 */
int ipc_connect(int argc, char** args);

/**
 * Método que permite enviar un datagrama a la otra parte involucrada
 * @param  data Datagrama a enviar
 * @return      Distinto de 0 en caso de error.
 */
int ipc_send(DB_DATAGRAM* data);

/**
 * Método que permite recibir un datagrama.
 * @return Copia del datagrama recibido. Es responsabilidad del usuario liberar la memoria.
 */
DB_DATAGRAM* ipc_receive();

/**
 * Método que permite desconectarse del servidor.
 */
void ipc_disconnect();

/**
 * Método que permite liberar los recursos usados por el servidor.
 */
void ipc_free();