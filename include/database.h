
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

typedef struct __attribute__((packed)){
	flight_id id;
	time_t departure;
	DB_SEAT seats[FLIGHT_SIZE];
	bool is_empty;
	airport_id origin;
	airport_id destination;
} DB_ENTRY;


typedef struct {

	size_t size;

	union{
		int 	_count;
		res_id  _seat;
		bool 	_result;
		int 	_shmemkey;
	} _data;
	
	union{
		DB_ENTRY** 	_results;
		char 		_cmd[1024];
	} _raw_data;

} DB_DATAGRAM;

#define dg_count 		_data._count 
#define dg_seat 		_data._seat 
#define dg_result 		_data._result 
#define dg_shmemkey 	_data._shmemkey

#define dg_cmd			_raw_data._cmd
#define dg_results		_raw_data._results

#define DUMP_DBENTRY(entry)			printf("Flight ID: %d\nDeparture: %lld\nOrigin: %d\n Destination: %d",\
										 entry->id, (long long)entry->departure, entry->origin, entry->destination)

#define DUMP_DATAGRAM(datagram)		{\
										printf("Size: %zu\nCount: %d\nSeat: %d\nResult: %s\n",\
											 datagram->size, datagram->dg_count, datagram->dg_seat,datagram->dg_result?"TRUE":"FALSE");\
									}
										//for(int __i = 0;__i<datagram->dg_count;__i++){\
										//	DUMP_DBENTRY(datagram->dg_results[__i]);\
										//}\
									//}

res_id purchase(flight_id id);

DB_DATAGRAM consult_flights(airport_id origin, airport_id destination);

bool cancel(res_id id);



#endif