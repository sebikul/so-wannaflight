
#ifndef DATABASE_H
#define DATABASE_H

#include <stddef.h>
#include <time.h>

#define FLIGHT_SIZE 32

//TODO alinear los datos a palabra.

typedef char bool;
//#define NULL (void*)0
typedef unsigned int flight_id;
typedef unsigned short airport_id;
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

typedef enum {OP_PURCHASE, OP_CONSULT, OP_CANCEL, OP_CMD, OP_EXIT, OP_CONNECT} OPCODE;

typedef struct {
	size_t size;
	OPCODE opcode;

	union {
		int 	_count;
		res_id  _seat;
		bool 	_result;
		int 	_shmemkey;
	} _data;

	union {
		DB_ENTRY 	_results[1];
		char 		_cmd[1];
	} _raw_data;

} DB_DATAGRAM;

#define dg_count 		_data._count
#define dg_seat 		_data._seat
#define dg_result 		_data._result
#define dg_shmemkey 	_data._shmemkey

#define dg_cmd			_raw_data._cmd
#define dg_results		_raw_data._results


#define DUMP_DBENTRY(entry)			printf("[DB_ENTRY][\n\t\tVuelo ID: %d\n\t\tSalida: %lld\n\t\tOrigen: %d\n\t\tDestino: %d\n]\n",\
										 entry.id, (long long)entry.departure, entry.origin, entry.destination)

#define DUMP_DATAGRAM(datagram)		printf("[DATAGRAM][\n\tTamaño: %zu\n\topcode: %d\n\tCantidad: %d\n\tAsiento: %d\n\tResultado: %s\n\tCMD: %s\n]\n",\
											 datagram->size, datagram->opcode, datagram->dg_count, datagram->dg_seat,datagram->dg_result?"TRUE":"FALSE",datagram->dg_cmd);\


#define DUMP_RESULT_DATAGRAM(datagram)	{\
											DUMP_DATAGRAM(datagram)\
											for(int i = 0; i < datagram->dg_count; i++){\
												DUMP_DBENTRY(datagram->dg_results[i]);\
											}\
										}

int db_open(const char* path);

void db_close();

res_id purchase(flight_id id);

DB_DATAGRAM* consult_flights(airport_id origin, airport_id destination);

void cancel(res_id id);

#endif
