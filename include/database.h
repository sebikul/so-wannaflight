
#ifndef DATABASE_H
#define DATABASE_H


#define FLIGHT_SIZE 32

//TODO alinear los datos a palabra.

typedef char bool;
typedef NULL (void*)0

typedef unsigned int flight_id;
typedef unsigned short airport_id;
typedef int res_id;

typedef struct {
	int last_flight_id;
	res_id last_reservation;
} DB_HEADER;


typedef struct {
	flight_id id;
	time_t departure;
	DB_SEAT seats[FLIGHT_SIZE];
	bool is_empty;
	airport_id destination;
	airport_id origin;
} DB_ENTRY __attribute__(packed);


typedef struct {
	int res_id;
} DB_SEAT;


typedef struct {
	union{
		int count;
		res_id seat;
		bool result;
	} data;
	
	DB_ENTRY* results;
} DB_DATAGRAM;

res_id purchase(flight_id id);

DB_RESULT consult_flights(airport_id origin, airport_id destination);

bool cancel(res_id id);



#endif