#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "database.h"
#include "../sqlite3/sqlite3.h"

#define printf(msg, ...) printf("[DATABASE] " msg, __VA_ARGS__);

static sqlite3 *db;

static int file_exist(const char* path) {
    return (access(path, 0) == 0);
}

static void db_create() {

    char* sql;
    char *err_msg ;
    int rc;


    sql = "CREATE TABLE ticket("
          "resid INT PRIMARY KEY     NOT NULL,"
          "id INT                    NOT NULL );"

          "CREATE TABLE flight("
          "id             INT PRIMARY KEY    NOT NULL,"
          "departure      INT                NOT NULL,"
          "origin         INT                NOT NULL,"
          "destination    INT                NOT NULL );";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK ) {

        printf("SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(db);

        exit(1);
    }


}

int db_open(const char* path) {

    int rc = sqlite3_open(path, &db);

    if (rc != SQLITE_OK) {
        printf("Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    } else {

        if(!file_exist(path)){
            printf("%s\n", "Database does not exist. Creating...");
            db_create();
        }
        
        printf("%s\n", "Opened database successfully");
    }
    return 0;
}

void db_close() {
    sqlite3_close(db);
}

res_id purchase(flight_id id) {

    // char* sql;
    // sprintf(sql, "SELECT COUNT(*) FROM ticket WHERE id = %d", id);
    // exec(sql);

    //TODO check if flight_id exist
    /*
    if(result< MAX && result > 0){
        add_ticket(id, RESERVATION_ID);
        RESERVATION_ID++; //GLOBAL VAR
    }
    */
}

DB_DATAGRAM* consult_flights(airport_id origin, airport_id destination) {

    char sql[128];
    sqlite3_stmt * statement;
    unsigned char * data;
    DB_DATAGRAM* datagram;
    int rowcount = 0;
    int rc;


    sprintf(sql, "SELECT * FROM flight WHERE origin = %d AND destination = %d", origin, destination);

    rc = sqlite3_prepare_v2(db, sql, -1, &statement, NULL);

    if (rc != SQLITE_OK) {
        printf("Couldn't prepare database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    datagram = malloc(DATAGRAM_MAXSIZE);
    datagram->opcode = OP_CONSULT;

    while ( (rc = sqlite3_step(statement)) == SQLITE_ROW) {

        DB_ENTRY entry;

        entry.id = sqlite3_column_int(statement, 0);
        entry.departure = sqlite3_column_int(statement, 1);
        entry.origin = sqlite3_column_int(statement, 2);
        entry.destination = sqlite3_column_int(statement, 3);

        printf("Adding flight %d to result.\n", entry.id);

        memcpy(&datagram->dg_results[rowcount], &entry, sizeof(DB_ENTRY)); 

        rowcount++;
    }

    datagram->dg_count = rowcount;
    datagram->size = sizeof(DB_DATAGRAM) + rowcount * sizeof(DB_ENTRY);

    //DUMP_RESULT_DATAGRAM(datagram);

    /* step() return SQLITE_DONE if it's out of records, but otherwise successful */
    if (rc != SQLITE_DONE) {
        /* this would indicate a problem */
        printf("Couldn't step through statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    /*
     * finalizing a statement is akin to freeing a variable made with malloc:
     * it frees memory, puts things away, and generally cleans house.
     * always run a finalize when you've finished with a given statement
     */
    rc = sqlite3_finalize(statement);
    if (rc) {
        printf("Couldn't finalize statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    //TODO work with callback result
}

bool cancel(res_id id) {
    // char* sql;
    // sprintf(sql, "SELECT ID from TICKET where RESID = %d", id);
    // exec(sql);

    //TODO search matching flightID from resID (only 1 result)
    /*
    if(result!=NULL){
        remove_ticket(id);
    }
    */
}

void add_flight(int flight_id, time_t ftime, int origin, int destination) {

    // char* sql;
    // sprintf(sql, "INSERT INTO FLIGHT (ID,TIME,ORIGIN,DESTINY) "  \
    //         "VALUES (%d,%d,%d,%d);", flight_id, ftime, origin, destination);

    // exec(sql);
}

void add_ticket(int flight_id, int reservation_id) {

    // char* sql;
    // sprintf(sql, "INSERT INTO TICKET (RESID, ID) "  \
    //         "VALUES (%d,%d);", reservation_id, flight_id);

    // exec(sql);
}

void remove_ticket(int reservation_id) {

    // char* sql;
    // sprintf(sql, "DELETE FROM TICKET where RESID = %d;", reservation_id);

    // exec(sql);
}

//TODO modify
static int callback(void *NotUsed, int argc, char **argv, char **azColName) {

    // int i;
    // for (i = 0; i < argc; i++) {
    //     printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    // }
    // printf("\n");
    // return 0;
}





int exec(char* sql) {

    // int rc; char *zErrMsg = 0;

    // rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    // if ( rc != SQLITE_OK ) {
    //     fprintf(stderr, "SQL error: %s\n", zErrMsg);
    //     sqlite3_free(zErrMsg);
    //     return 1;

    // } else {
    //     fprintf(stdout, "Table created successfully\n");
    //     return 0;
    // }
}



