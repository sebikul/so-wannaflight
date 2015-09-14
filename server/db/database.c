#include "database.h"
#include "sqlite3/sqlite3.h"

sqlite3 *db;

res_id purchase(flight_id id){
    
    char* sql;
    sprintf(sql,"SELECT COUNT(*) from TICKET where ID = %d",id);
    exec(sql);  

    //TODO check if flight_id exist
    /*
    if(result< MAX && result > 0){
        add_ticket(id, RESERVATION_ID);
        RESERVATION_ID++; //GLOBAL VAR
    }
    */  
}

DB_DATAGRAM consult_flights(airport_id origin, airport_id destination){

    char* sql;
    sprintf(sql,"SELECT * from FLIGHT where ORIGIN = %d AND DESTINY = %d",origin, destination);
    exec(sql);
    
    //TODO work with callback result
}

bool cancel(res_id id){
    char* sql;
    sprintf(sql,"SELECT ID from TICKET where RESID = %d", id);
    exec(sql);

    //TODO search matching flightID from resID (only 1 result)
    /*
    if(result!=NULL){
        remove_ticket(id);
    }
    */
}

void add_flight(int flight_id, time_t ftime, int origin, int destination){

    char* sql;
    sprintf(sql,"INSERT INTO FLIGHT (ID,TIME,ORIGIN,DESTINY) "  \
         "VALUES (%d,%d,%d,%d);", flight_id,ftime,origin,destination);

    exec(sql);
}

void add_ticket(int flight_id, int reservation_id){

    char* sql;
    sprintf(sql,"INSERT INTO TICKET (RESID, ID) "  \
         "VALUES (%d,%d);", reservation_id, flight_id);

    exec(sql);
}

void remove_ticket(int reservation_id){

    char* sql;
    sprintf(sql,"DELETE FROM TICKET where RESID = %d;", reservation_id);

    exec(sql);
}

//TODO modify
static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    
    int i;
    for(i=0; i<argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int open(){

    int  rc;char *zErrMsg = 0;

    rc = sqlite3_open(NULL, &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }else{
        create();
        fprintf(stdout, "Opened database successfully\n");
    }
    return 0;
}

void close(){
    sqlite3_close(db);
}

int exec(char* sql){

    int rc;char *zErrMsg = 0;

    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 1;

    }else{
        fprintf(stdout, "Table created successfully\n");
        return 0;
    }
}

void create(){

    char* sql;
    sql = "CREATE TABLE TICKET("  \
        "RESID INT PRIMARY KEY     NOT NULL," \
        "ID INT                    NOT NULL );";

    exec(sql);

    sql = "CREATE TABLE FLIGHT("  \
         "ID             INT PRIMARY KEY    NOT NULL," \
         "TIME           TIMESTAMP          NOT NULL," \
         "ORIGIN         INT                NOT NULL," \
         "DESTINY        INT                NOT NULL );";

    exec(sql);
}

