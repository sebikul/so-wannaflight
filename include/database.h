
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
	airport_id destination;
	airport_id origin;
} DB_ENTRY;


typedef struct {
	union{
		int count;
		res_id seat;
		bool result;
	} data;
	
	DB_ENTRY* results;
} DB_DATAGRAM;

res_id purchase(flight_id id);

DB_DATAGRAM consult_flights(airport_id origin, airport_id destination);

bool cancel(res_id id);



#endif