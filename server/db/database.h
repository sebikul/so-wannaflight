
#define FLIGHT_SIZE 32

//TODO alinear los datos a palabra.

typedef char bool;

typedef unsigned int flight_id;
typedef unsigned short airport_id;
typedef int reservation_id;

typedef struct {
	int last_flight_id;
	reservation_id last_reservation;
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
	int reservation_id;
} DB_SEAT;


typedef struct {
	int count;
	DB_ENTRY* results;
} DB_RESUL;

reservation_id purchase(flight_id id);

DB_RESULT consult_flights(airport_id origin, airport_id destination);

bool cancel(reservation_id id);
