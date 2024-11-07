#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "SQLParser.h"

void initSQLParser(SQLParser *parser)
{
    parser->parse = parse;
    parser->parseSelect = parseSelect;
    parser->parseInsert = parseInsert;
    parser->parseUpdate = parseUpdate;
    parser->parseDelete = parseDelete;
    parser->parseCreateTable = parseCreateTable;
}

int parse(SQLParser *parser, const char *query, char* title)
{
    char command[MAX_LENGTH];
    char stream[MAX_LENGTH];

    sscanf(query, "%s %[^\n]", command, stream);

    for (int i = 0; command[i]; i++)
    {
        command[i] = toupper(command[i]);
    }

    if (strcmp(command, "SELECT") == 0)
    {
        parser->parseSelect(parser, stream,title);
        return 0;
    }
    else if (strcmp(command, "INSERT") == 0)
    {
        parser->parseInsert(parser, stream);
        return 1;
    }
    else if (strcmp(command, "UPDATE") == 0)
    {
        parser->parseUpdate(parser, stream);
        return 2;
    }
    else if (strcmp(command, "DELETE") == 0)
    {
        parser->parseDelete(parser, stream);
        return 3;
    }
    else if (strcmp(command, "CREATE") == 0)
    {
        parser->parseCreateTable(parser, stream);
        return 4;
    }
    else
    {
        printf("Unknown command: %s\n", command);
        return -1;
    }
}

char** parseSelect(SQLParser *parser, char *stream,char* tableName)
{
    char** columns = (char**)malloc(MAX_COLUMNS * sizeof(char*));
    if (columns == NULL) {
        printf("Eroare la alocarea memoriei pentru coloane.\n");
        return NULL;
    }

    int colCount = 0;
    char* token = strtok(stream, " ");
    token = strtok(NULL, " ");
    while (token != NULL && strcmp(token, "FROM") != 0) {
        columns[colCount] = (char*)malloc(MAX_LENGTH * sizeof(char));
        if (columns[colCount] == NULL) {
            printf("Eroare la alocarea memoriei pentru coloană.\n");
            return NULL;
        }
        strncpy(columns[colCount], token, MAX_LENGTH - 1);
        columns[colCount][MAX_LENGTH - 1] = '\0';

        int len = strlen(columns[colCount]);
        if (columns[colCount][len - 1] == ',') {
            columns[colCount][len - 1] = '\0';
        }

        colCount++;
        token = strtok(NULL, ", ");
    }

    char from[MAX_LENGTH], table[MAX_LENGTH];
    token = strtok(NULL, " ");
    strncpy(table, token, MAX_LENGTH - 1);
    strcpy(tableName, table);
    printf("SELECT command: Columns = ");
    for (int i = 0; i < colCount; i++) {
        printf("%s ", columns[i]);
    }
    printf(", Table = %s\n", table);

    return columns;
}

Column* parseInsert(SQLParser *parser, char *stream)
{
    Column coloane[20];
    char into[MAX_LENGTH], table[MAX_LENGTH];
    char columns[MAX_COLUMNS][MAX_LENGTH];
    char values[MAX_VALUES][MAX_LENGTH];
    int col = 0, val = 0;
    sscanf(stream, "%s %s", into, table);
    char *columns_start = strchr(stream, '(');
    char *values_start = strstr(stream, "VALUES");
    if (columns_start && values_start)
    {
        columns_start++;
        while (*columns_start != ')' && *columns_start != '\0')
        {
            sscanf(columns_start, "%[^,)]", columns[col]);
            columns_start += strlen(columns[col]);
            if (*columns_start == ',')
                columns_start++;
            col++;
        }
        values_start = strchr(values_start, '(');
        if (values_start)
        {
            values_start++;
            while (*values_start != ')' && *values_start != '\0')
            {
                sscanf(values_start, "%[^,)]", values[val]);
                values_start += strlen(values[val]);
                if (*values_start == ',')
                    values_start++;
                val++;
            }
        }
        printf("INSERT command: Table = %s\nColumns: ", table);
        for (int i = 0; i < col; i++)
        {
            printf("%s ", columns[i]);
        }

        printf("\nValues: ");
        for (int i = 0; i < val; i++)
        {
            printf("%s ", values[i]);
        }
        printf("\n");
    }
    else
    {
        printf("Invalid INSERT syntax.\n");
    }
    return coloane;
}

void parseUpdate(SQLParser *parser, char *stream)
{
    char table[MAX_LENGTH];
    char set[MAX_LENGTH] = "";
    char condition[MAX_LENGTH] = "";
    sscanf(stream, "%s", table);
    char *set_start = strstr(stream, "SET");
    if (set_start != NULL)
    {
        set_start += 4;
        char *where_start = strstr(set_start, "WHERE");

        if (where_start != NULL)
        {
            strncpy(set, set_start, where_start - set_start);
            set[where_start - set_start] = '\0';
            strncpy(condition, where_start, MAX_LENGTH - 1);
            condition[MAX_LENGTH - 1] = '\0';
        }
        else
        {
            strncpy(set, set_start, MAX_LENGTH - 1);
            set[MAX_LENGTH - 1] = '\0';
        }
    }
    printf("UPDATE command: Table = %s, Set = %s, Condition = %s\n", table, set, condition);
}

void parseCreateTable(SQLParser *parser, char *stream)
{
    char *token = strtok(stream, " ");
    token = strtok(NULL, " ");
    token = strtok(NULL, " ");
    char numeTabel[1024];
    strncpy(numeTabel, token, 1024);
    int numarColoane = 0;
    char numeColoane[20][40];
    char tipuriDate[20][10];

    while ((token = strtok(NULL, " ,()")) != NULL)
    {
        strncpy(numeColoane[numarColoane], token, 40);
        token = strtok(NULL, " ,()");
        if (strcmp(token, "INT") == 0)
        {
            strcpy(tipuriDate[numarColoane], "INT");
        }
        else if (strcmp(token, "VARCHAR") == 0)
        {
            strcpy(tipuriDate[numarColoane], "VARCHAR");
            token = strtok(NULL, " ,()");
            // aici de gestionat daca e varchar
            // t->coloane[t->numarColoane].varchar_length = atoi(token);
        }
        else if (strcmp(token, "DATE") == 0)
        {
            strcpy(tipuriDate[numarColoane], "DATE");
        }

        numarColoane++;
    }
    printf("Tabel creat: %s\n", numeTabel);
    for (int i = 0; i < numarColoane; i++)
    {
        printf("Coloana %d: %s, Tip: ", i + 1, numeColoane[i]);
        printf("\t%s",tipuriDate[i]);
    }
}

void parseDelete(SQLParser *parser, char *stream)
{
    char from[MAX_LENGTH], table[MAX_LENGTH];
    char condition[MAX_LENGTH] = "";
    sscanf(stream, "%s %s", from, table);
    char *condition_start = strstr(stream, "WHERE");
    if (condition_start != NULL)
    {
        strncpy(condition, condition_start, MAX_LENGTH - 1);
        condition[MAX_LENGTH - 1] = '\0';
    }
    printf("DELETE command: Table = %s, Condition = %s\n", table, condition);
}
