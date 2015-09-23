
#ifndef DATABASE_H
#define DATABASE_H

#include <stddef.h>
#include <time.h>

#define FLIGHT_SIZE 32

//TODO alinear los datos a palabra.

//typedef char bool;
typedef enum {FALSE, TRUE} bool;
//#define NULL (void*)0
typedef unsigned int flight_id;
typedef int airport_id;
typedef int res_id;

typedef struct {
	int last_flight_id;
	res_id last_reservation;
} DB_HEADER;

typedef struct {
	int res_id;
} DB_SEAT;

typedef struct __attribute__((packed)) {
	flight_id id;
	time_t departure;
	DB_SEAT seats[FLIGHT_SIZE];
	bool is_empty;
	airport_id origin;
	airport_id destination;
} DB_ENTRY;

typedef enum {OP_PURCHASE, OP_CONSULT, OP_CANCEL, OP_EXIT, OP_CONNECT, OP_PING, OP_PONG, OP_ADDFLIGHT} OPCODE;

typedef struct {
	size_t size;
	OPCODE opcode;

#ifdef MSGQUEUE
	int sender;
#endif

	union {
		int 		_count;
		res_id  	_seat;
		bool 		_result;
		int 		_shmemkey;
		flight_id 	_flight_id;
		int 		_origin;
	} _data;

	union {
		DB_ENTRY 	_results[1];
		char 		_cmd[1];
		int 		_destination;
	} _raw_data;

} DB_DATAGRAM;

#define dg_count 		_data._count
#define dg_seat 		_data._seat
#define dg_result 		_data._result
#define dg_shmemkey 	_data._shmemkey
#define dg_flightid		_data._flight_id
#define dg_origin		_data._origin

#define dg_cmd			_raw_data._cmd
#define dg_results		_raw_data._results
#define dg_destination  _raw_data._destination


#define DUMP_DBENTRY(entry)			printf("[DB_ENTRY][\n\t\tVuelo ID: %d\n\t\tSalida: %lld\n\t\tOrigen: %d\n\t\tDestino: %d\n]\n",\
										 entry.id, (long long)entry.departure, entry.origin, entry.destination)

#ifdef MSGQUEUE
#define DATAGRAM_PRINT(datagram) printf("[DATAGRAM][\n\tTamaño: %zu\n\topcode: %d\n\tsender: %d\n", datagram->size, datagram->opcode, datagram->sender);
#else
#define DATAGRAM_PRINT(datagram) printf("[DATAGRAM][\n\tTamaño: %zu\n\topcode: %d\n", datagram->size, datagram->opcode);
#endif

#define DUMP_DATAGRAM(datagram)		{\
										DATAGRAM_PRINT(datagram)\
										if(datagram->opcode==OP_CONSULT){\
											printf("\tCantidad: %d\n\tOrigen: %d\n\tDestino: %d\n", datagram->dg_count, datagram->dg_origin, datagram->dg_destination);\
											DUMP_RESULT_DATAGRAM(datagram);\
										}else if(datagram->opcode==OP_PING || datagram->opcode==OP_PONG){\
											printf("\tCMD: %s\n", datagram->dg_cmd);\
										}\
										printf("]\n");\
									}

#define DUMP_RESULT_DATAGRAM(datagram)	{\
											for(int i = 0; i < datagram->dg_count; i++){\
												DUMP_DBENTRY(datagram->dg_results[i]);\
											}\
										}

int db_open(const char* path);

void db_close();

res_id db_purchase(flight_id id);

DB_DATAGRAM* db_consult_flights(airport_id origin, airport_id destination);

bool db_cancel(res_id id);

int db_add_flight(time_t departure, int origin, int destination);

#endif
