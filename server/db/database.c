#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "database.h"
#include "../sqlite3/sqlite3.h"

#define printf(msg, ...) printf("[DATABASE] " msg, __VA_ARGS__);

#define DB_PREPARE_STATEMENT(db, query, statement) {\
                                                        int rc = sqlite3_prepare_v2(db, query, -1, &statement, NULL);\
                                                        if (rc != SQLITE_OK) {\
                                                            printf("Couldn't prepare database: %s\n", sqlite3_errmsg(db));\
                                                            sqlite3_close(db);\
                                                            exit(1);\
                                                        }\
                                                   }
#define DB_FINALIZE_STATEMENT(db)   {\
                                        int rc = sqlite3_finalize(statement);\
                                        if (rc != SQLITE_OK) {\
                                            printf("Couldn't finalize statement: %s\n", sqlite3_errmsg(db));\
                                            sqlite3_close(db);\
                                            exit(1);\
                                        }\
                                    }

#define DB_EXEC(db, query)      {\
                                    int rc;\
                                    char* err_msg;\
                                    rc = sqlite3_exec(db, query, 0, 0, &err_msg);\
                                    if (rc != SQLITE_OK ) {\
                                        printf("SQL error: %s\n", err_msg);\
                                        sqlite3_free(err_msg);\
                                        sqlite3_close(db);\
                                        exit(1);\
                                    }\
                                }

#define SQLITE_STATEMENT_EXPECT(rc, expect)      if (rc != expect) {\
                                                    printf("Couldn't step through statement: %s\n", sqlite3_errmsg(db));\
                                                    sqlite3_close(db);\
                                                    exit(1);\
                                                }

#define DB_COL_FLIGHTID        0
#define DB_COL_DEPARTURE       1
#define DB_COL_ORIGIN          2
#define DB_COL_DESTINATION     3

static sqlite3 *db;

static int file_exist(const char* path) {
    return (access(path, 0) == 0);
}

static void db_create() {

    char* sql;
    char *err_msg ;
    int rc;


    sql = "CREATE TABLE ticket("
          "resid    INT  PRIMARY KEY NOT NULL,"
          "flightid INT              NOT NULL );"

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

static int db_flight_exists(flight_id id) {

    char query[128];
    sqlite3_stmt * statement;
    int rc, rowcount = 0;

    sprintf(query, "SELECT COUNT(*) FROM flight WHERE id = %d ", id);

    DB_PREPARE_STATEMENT(db, query, statement);

    rc = sqlite3_step(statement);
    SQLITE_STATEMENT_EXPECT(rc, SQLITE_ROW);

    rowcount = sqlite3_column_int(statement, 0);

    rc = sqlite3_step(statement);
    SQLITE_STATEMENT_EXPECT(rc, SQLITE_DONE);

    DB_FINALIZE_STATEMENT(db);

    return (rowcount == 1);
}

int db_open(const char* path) {

    int has_to_populate = 0;
    int rc;

    if (!file_exist(path)) {
        has_to_populate = 1;
    }

    rc = sqlite3_open(path, &db);

    if (rc != SQLITE_OK) {
        printf("Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    } else {

        if (has_to_populate) {
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

static int add_flight(time_t departure, int origin, int destination) {

    char query[128];

    sprintf(query, "INSERT INTO flight (departure,origin,destination) VALUES (%ld,%d,%d);", departure, origin, destination);

    DB_EXEC(db, query);

    return sqlite3_last_insert_rowid(db);
}

static int add_ticket(flight_id id) {

    char query[128];

    sprintf(query, "INSERT INTO ticket (flightid) VALUES (%d);", id);

    DB_EXEC(db, query);

    return sqlite3_last_insert_rowid(db);
}

static void remove_ticket(res_id id) {

    char query[128];

    sprintf(query, "DELETE FROM ticket where resid = %d;", id);

    DB_EXEC(db, query);
}


res_id purchase(flight_id id) {

    if (!db_flight_exists(id)) {
        return -1;
    }

    return add_ticket(id);
}

DB_DATAGRAM* consult_flights(airport_id origin, airport_id destination) {

    char query[128] = {0};
    sqlite3_stmt * statement;
    DB_DATAGRAM* datagram;
    int rowcount = 0;
    int rc;

    sprintf(query, "SELECT * FROM flight WHERE origin = %d AND destination = %d", origin, destination);

    DB_PREPARE_STATEMENT(db, query, statement);

    datagram = malloc(DATAGRAM_MAXSIZE);

    while ( (rc = sqlite3_step(statement)) == SQLITE_ROW) {

        DB_ENTRY entry;

        entry.id = sqlite3_column_int(statement, DB_COL_FLIGHTID);
        entry.departure = sqlite3_column_int(statement, DB_COL_DEPARTURE);
        entry.origin = sqlite3_column_int(statement, DB_COL_ORIGIN);
        entry.destination = sqlite3_column_int(statement, DB_COL_DESTINATION);

        printf("Adding flight %d to result.\n", entry.id);

        memcpy(&datagram->dg_results[rowcount], &entry, sizeof(DB_ENTRY));

        rowcount++;
    }

    if (rc != SQLITE_DONE) {
        printf("Couldn't step through statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    DB_FINALIZE_STATEMENT(db);

    datagram->dg_count = rowcount;
    datagram->size = sizeof(DB_DATAGRAM) + rowcount * sizeof(DB_ENTRY);
    datagram->opcode = OP_CONSULT;

    datagram = realloc(datagram, datagram->size);

    return datagram;
}

void cancel(res_id id) {

    remove_ticket(id);
}


