#include "SQLParser.h"
#include <stdio.h>

int main()
{
    char query[] = "CREATE TABLE angajati (id INT , nume VARCHAR, varsta INT, data_angajarii DATE);";
    SQLParser *parser = (SQLParser *)malloc(sizeof(SQLParser)); 
    initSQLParser(parser);
    char title[20];
    // parser.parseSelect = parseSelect;
    Table* t = parser->parseCreateTable(&parser, query);
    printf("\n---------------------------------------\n");
    for (int i = 0; i< t->numarColoane ; i++)
    {
        printf("%s\n", t->coloane[i].numeColoana);
    }
    return 0;
}
