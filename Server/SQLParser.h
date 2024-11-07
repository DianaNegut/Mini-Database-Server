#ifndef SQLPARSER_H
#define SQLPARSER_H

#define MAX_COLUMNS 20
#define MAX_VALUES 20
#define MAX_LENGTH 100
#include "gestionareTabele.h"


typedef struct SQLParser {
    int (*parse)(struct SQLParser*, const char*,char *);
    char** (*parseSelect)(struct SQLParser*, char*, char*);
    Column* (*parseInsert)(struct SQLParser*, char*);
    void (*parseUpdate)(struct SQLParser*, char*);
    void (*parseDelete)(struct SQLParser*, char*);
    void (*parseCreateTable)(struct SQLParser*, char*);
} SQLParser;


void initSQLParser(SQLParser* parser);
int parse(SQLParser* parser, const char* query, char* tableName);
char** parseSelect(SQLParser* parser, char* stream,char* tableName);
Column* parseInsert(SQLParser* parser, char* stream);
void parseUpdate(SQLParser* parser, char* stream);
void parseDelete(SQLParser* parser, char* stream);
void parseCreateTable(SQLParser* parser, char* stream);

#endif // SQLPARSER_H
