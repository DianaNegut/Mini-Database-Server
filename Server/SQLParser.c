#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
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

int parse(SQLParser *parser, const char *query)
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
        return 0;
    }
    else if (strcmp(command, "INSERT") == 0)
    {
        return 1;
    }
    else if (strcmp(command, "UPDATE") == 0)
    {
        return 2;
    }
    else if (strcmp(command, "DELETE") == 0)
    {
        return 3;
    }
    else if (strcmp(command, "CREATE") == 0)
    {
        return 4;
    }
    else
    {
        printf("Unknown command: %s\n", command);
        return -1;
    }
}

char **parseSelect(SQLParser *parser, char *stream, char *tableName)
{
    char **columns = (char **)malloc(MAX_COLUMNS * sizeof(char *));
    if (columns == NULL)
    {
        printf("Eroare la alocarea memoriei pentru coloane.\n");
        return NULL;
    }

    int colCount = 0;
    char *token = strtok(stream, " ");
    token = strtok(NULL, " ");
    while (token != NULL && strcmp(token, "FROM") != 0)
    {
        columns[colCount] = (char *)malloc(MAX_LENGTH * sizeof(char));
        if (columns[colCount] == NULL)
        {
            printf("Eroare la alocarea memoriei pentru coloană.\n");
            return NULL;
        }
        strncpy(columns[colCount], token, MAX_LENGTH - 1);
        columns[colCount][MAX_LENGTH - 1] = '\0';

        int len = strlen(columns[colCount]);
        if (columns[colCount][len - 1] == ',')
        {
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
    for (int i = 0; i < colCount; i++)
    {
        printf("%s ", columns[i]);
    }
    printf(", Table = %s\n", table);

    return columns;
}

bool exists_in_master(const char *master_filename, const char *entry){
    FILE *file = fopen(master_filename, "r");
    if (file == NULL)
    {
        perror("Eroare la deschiderea fișierului master");
        return false;
    }
    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, entry) == 0)
        {
            fclose(file);
            return true;
        }
    }
    fclose(file);
    return false;
}


void append_to_file_values(const char *filename, char values[][MAX_LENGTH], int num_values) {
    int fd = open(filename, O_WRONLY | O_APPEND);
    if (fd == -1) {
        perror("Eroare la deschiderea fișierului");
        return;
    }
    for (int i = 0; i < num_values; i++) {
        // Scriem valoarea în fișier
        if (write(fd, values[i], strlen(values[i])) == -1) {
            perror("Eroare la scrierea valorii în fișier");
            close(fd);
            return;
        }
        if (i < num_values - 1) {
            if (write(fd, " ", 1) == -1) {
                perror("Eroare la adăugarea spațiului");
                close(fd);
                return;
            }
        }
    }
    if (write(fd, "\n", 1) == -1) {
        perror("Eroare la adăugarea newline-ului");
        close(fd);
        return;
    }
    if (close(fd) == -1) {
        perror("Eroare la închiderea fișierului");
    }
}

void cleanBuffer(char *buffer) {
    char *src = buffer;
    char *dst = buffer;
    
    while (*src) {
        if (*src != '\000') {  
            *dst++ = *src;  
        }
        src++;
    }
    *dst = '\0';  
}

void parseInsert(SQLParser *parser, char *stream)
{
    char insert[MAX_LENGTH];
    char into[MAX_LENGTH], table[MAX_LENGTH];
    char columns[MAX_COLUMNS][MAX_LENGTH];
    char values[MAX_VALUES][MAX_LENGTH];
    int col = 0, val = 0;
    cleanBuffer(stream);
    sscanf(stream, "%s%s%s", insert, into, table);
    cleanBuffer(insert);
    cleanBuffer(into);
    cleanBuffer(table);
    if (!exists_in_master("master", table))
    {
        perror("Eroare, tabelul nu exista in baza de date!");
        exit(-1);
    }
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
        append_to_file_values(table, values, val);
        printf("\n");
    }
    else
    {
        printf("Invalid INSERT syntax.\n");
    }
}

void parseUpdate(SQLParser *parser, char *stream)
{
    char table[MAX_LENGTH];
    char set[MAX_LENGTH] = "";
    char condition[MAX_LENGTH] = "";
    sscanf(stream, "%s", table);
    if (!exists_in_master("master", table))
    {
        perror("Eroare, tabelul nu exista in baza de date!");
        exit(-1);
    }
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

void append_to_file(const char *filename, const char *text){
    int fd = open(filename, O_WRONLY | O_APPEND);
    if (fd == -1)
    {
        perror("Eroare la deschiderea fișierului");
        return;
    }
    const char *newline = "\n";
    if (write(fd, newline, strlen(newline)) == -1)
    {
        perror("Eroare la scrierea newline-ului");
        close(fd);
        return;
    }
    if (write(fd, text, strlen(text)) == -1)
    {
        perror("Eroare la scrierea textului");
        close(fd);
        return;
    }
    if (close(fd) == -1)
    {
        perror("Eroare la închiderea fișierului");
    }
}


void creareFisier(const char* filename)
{
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);  
    if (fd == -1) {
        perror("Eroare la crearea fișierului");
        return 1;  
    }
    if (close(fd) == -1) {
        perror("Eroare la închiderea fișierului");
        return 1;  
    }
}

Table *parseCreateTable(SQLParser *parser, char *stream)
{
    Table *t = (Table *)malloc(sizeof(Table));
    char *token = strtok(stream, " ");
    token = strtok(NULL, " ");
    token = strtok(NULL, " ");
    char numeTabel[1024];
    strncpy(numeTabel, token, 1024);
    Column *c = (Column *)malloc(20 * sizeof(Column));
    int numarColoane = 0;
    char numeColoane[20][40];
    char tipuriDate[20][10];
    strcpy(t->numeTabel, numeTabel);
    creareFisier(t->numeTabel);
    if (!exists_in_master("master", t->numeTabel))
        append_to_file("master", t->numeTabel);
    while ((token = strtok(NULL, " ,()")) != NULL)
    {
        strncpy(c[numarColoane].numeColoana, token, 40);
        strncpy(numeColoane[numarColoane], token, 40);
        token = strtok(NULL, " ,()");
        if (strcmp(token, "INT") == 0)
        {
            strcpy(c[numarColoane].tipDate, "INT");
            strcpy(tipuriDate[numarColoane], "INT");
        }
        else if (strcmp(token, "VARCHAR") == 0)
        {
            strcpy(c[numarColoane].tipDate, "VARCHAR");
            strcpy(tipuriDate[numarColoane], "VARCHAR");
            token = strtok(NULL, " ,()");
            // aici de gestionat daca e varchar
            // t->coloane[t->numarColoane].varchar_length = atoi(token);
        }
        else if (strcmp(token, "DATE") == 0)
        {
            strcpy(c[numarColoane].tipDate, "DATE");
            strcpy(tipuriDate[numarColoane], "DATE");
        }

        numarColoane++;
    }
    t->coloane = c;
    t->randuri = 0;

    t->numarColoane = numarColoane;
    printf("Tabel creat: %s\n", numeTabel);
    for (int i = 0; i < numarColoane; i++)
    {
        printf("Coloana %d: %s, Tip: ", i + 1, numeColoane[i]);
        printf("\t%s", tipuriDate[i]);
    }
    return t;
}

void parseDelete(SQLParser *parser, char *stream)
{
    char from[MAX_LENGTH], table[MAX_LENGTH];
    char condition[MAX_LENGTH] = "";
    sscanf(stream, "%s %s", from, table);
    if (!exists_in_master("master", table))
    {
        perror("Eroare, tabelul nu exista in baza de date!");
        exit(-1);
    }
    char *condition_start = strstr(stream, "WHERE");
    if (condition_start != NULL)
    {
        strncpy(condition, condition_start, MAX_LENGTH - 1);
        condition[MAX_LENGTH - 1] = '\0';
    }
    printf("DELETE command: Table = %s, Condition = %s\n", table, condition);
}
