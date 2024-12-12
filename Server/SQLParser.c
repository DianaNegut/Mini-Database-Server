#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "SQLParser.h"


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


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

char **parseSelect(SQLParser *parser, char *stream, char *tableName, char *whereColumn, char *whereValue, char *whereop)
{
    char **columns = (char **)malloc(MAX_COLUMNS * sizeof(char *));
    if (columns == NULL)
    {
        printf("Eroare la alocarea memoriei pentru coloane.\n");
        return NULL;
    }

    // operator
    char *buff = strdup(stream);
    char *tkn = strtok(buff, " ");
    while (tkn != NULL && strchr(tkn, '=') == 0 && strchr(tkn, '<') == 0 && strchr(tkn, '>') == 0)
    {
        tkn = strtok(NULL, " ");
    }

    if (isalnum(tkn[0]))
    {
        int i = 1;
        while (isalnum(tkn[i]))
        {
            i++;
        }
        whereop[0] = tkn[i];
        if (tkn[i + 1] == '=')
        {
            whereop[1] = tkn[i + 1];
            whereop[2] = '\0';
        }
        else
            whereop[1] = '\0';
    }
    else
    {
        whereop[0] = tkn[0];
        if (tkn[1] != ' ' && tkn[1] != '\"')
        {
            whereop[1] = tkn[1];
            whereop[2] = '\0';
        }
        else
            whereop[1] = '\0';
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
        strncpy(columns[colCount], token, strlen(token));
        columns[colCount][strlen(token)] = '\0';

        int len = strlen(columns[colCount]);
        if (columns[colCount][len - 1] == ',')
        {
            columns[colCount][len - 1] = '\0';
        }

        colCount++;
        token = strtok(NULL, ", ");
    }
    columns[colCount] = NULL;
    char from[MAX_LENGTH], table[MAX_LENGTH];
    token = strtok(NULL, " ");
    strncpy(table, token, strlen(token));
    strcpy(tableName, table);

    token = strtok(NULL, " ");
    if (token != NULL && strcmp(token, "WHERE") == 0)
    {

        token = strtok(NULL, " <=>");

        if (token != NULL)
        {
            strncpy(whereColumn, token, strlen(token));
            whereColumn[strlen(token)] = '\0';
        }

        token = strtok(NULL, " <=>");

        if (token != NULL)
        {
            strncpy(whereValue, token, strlen(token));
            whereValue[strlen(token)] = '\0';
        }
    }

    printf("SELECT command: Columns = ");
    for (int i = 0; i < colCount; i++)
    {
        printf("%s ", columns[i]);
    }
    printf(", Table = %s\n", table);

    if (strlen(whereColumn) > 0 && strlen(whereValue) > 0)
    {
        printf(", WHERE %s %s \"%s\"", whereColumn, whereop, whereValue);
    }

    printf("\n-----------------------\n\n");

    return columns;
}

bool exists_in_master(const char *master_filename, const char *entry)
{
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

void append_to_file_values(const char *filename, char values[][MAX_LENGTH], int num_values)
{
    int fd = open(filename, O_WRONLY | O_APPEND);
    if (fd == -1)
    {
        perror("Eroare la deschiderea fișierului");
        return;
    }
    for (int i = 0; i < num_values; i++)
    {
        // Scriem valoarea în fișier
        if (write(fd, values[i], strlen(values[i])) == -1)
        {
            perror("Eroare la scrierea valorii în fișier");
            close(fd);
            return;
        }
        if (i < num_values - 1)
        {
            if (write(fd, " ", 1) == -1)
            {
                perror("Eroare la adăugarea spațiului");
                close(fd);
                return;
            }
        }
        write(fd, "\t", 1);
    }
    if (write(fd, "\n", 1) == -1)
    {
        perror("Eroare la adăugarea newline-ului");
        close(fd);
        return;
    }
    if (close(fd) == -1)
    {
        perror("Eroare la închiderea fișierului");
    }
}

void cleanBuffer(char *buffer)
{
    char *src = buffer;
    char *dst = buffer;

    while (*src)
    {
        if (*src != '\000')
        {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

void parseInsert(SQLParser *parser, char *stream, int socket)
{
    Table *tabel;
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
    // if (!exists_in_master("master", table))
    // {
    //     perror("Eroare, tabelul nu exista in baza de date!");
    //     exit(-1);
    // }
    pthread_mutex_lock(&mutex);
    tabel = loadTable(table);
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
                if (*values_start == '\"') {
                    values[val][0] = '\"';
                    values_start++;
                    sscanf(values_start, "%[^\"]", values[val]+1);
                    int n = strlen(values[val]);
                    values[val][n] = '\"';
                    values[val][n+1] = '\0';
                } 
                else {
                // Citim valoarea fără ghilimele
                sscanf(values_start, "%[^, )]", values[val]);
                }
                // verif datele inserate
                if (strcmp(tabel->coloane[val].tipDate, "INT") == 0)
                {
                    for (int i = 0; i < strlen(values[val]); i++)
                        if (!isdigit(values[val][i]))
                        {
                            char buffer_auxiliar[BUFSIZ];
                            snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "Valoarea care se incearca a fi inserata INT nu corespunde cu tipul de date al coloanei! %s", tabel->coloane[val].tipDate);
                            send(socket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
                            printf("Valoarea care se incearca a fi inserata INT nu corespunde cu tipul de date al coloanei! %s", tabel->coloane[val].tipDate);
                            exit(-1);
                        }
                }
                else if (strcmp(tabel->coloane[val].tipDate, "VARCHAR") == 0)
                {
                    if (strlen(values[val]) > tabel->coloane[val].varchar_length)
                    {
                        char buffer_auxiliar[BUFSIZ];
                        snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "Valoarea care se incearca a fi inserata are o lungime mai mare decat cea permisa de aceasta coloana!");
                        send(socket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
                        printf("Valoarea care se incearca a fi inserata are o lungime mai mare decat cea permisa de aceasta coloana!");
                        exit(-1);
                    }
                }
                else if (strcmp(tabel->coloane[val].tipDate, "DATE") == 0)
                {
                }
                values_start += strlen(values[val]);
                if (*values_start == ',')
                    values_start++;
                if(*values_start == ' ')
                    values_start++;
                val++;
            }
        }
        char buffer_auxiliar[BUFSIZ];
        snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "INSERT command: Table = %s\nColumns: ", table);
        send(socket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
        printf("INSERT command: Table = %s\nColumns: ", table);
        for (int i = 0; i < col; i++)
        {
            snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "%s ", columns[i]);
            send(socket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
            printf("%s ", columns[i]);
        }

        printf("\nValues: ");

        for (int i = 0; i < val; i++)
        {
            printf("%s ", values[i]);
            snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "%s ", values[i]);
            send(socket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
        }
        append_to_file_values(table, values, val);
        send(socket, "\n", strlen("\n"), 0);
        printf("\n");
        pthread_mutex_unlock(&mutex);
    }
    else
    {
        char buffer_auxiliar[BUFSIZ];
        snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "Invalid INSERT syntax.\n");
        send(socket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
        printf("Invalid INSERT syntax.\n");
        pthread_mutex_unlock(&mutex);
    }
}

void parseUpdate(SQLParser *parser, char *stream, char *tableName, char *setcol, char *setval, char *wherecol, char *whereval, char *whereop)
{

    char *token = strtok(stream, " ");

    if (token == NULL || strcmp(token, "UPDATE") != 0)
    {
        exit(-1);
    }

    token = strtok(NULL, " ");
    if (token == NULL)
    {
        exit(-1);
    }
    strncpy(tableName, token, strlen(token));
    tableName[strlen(token)] = '\0';

    if (!exists_in_master("master", tableName))
    {
        exit(-1);
    }

    token = strtok(NULL, " ");
    if (token == NULL || strcmp(token, "SET") != 0)
    {
        exit(-1);
    }

    token = strtok(NULL, "=");
    if (token == NULL)
    {
        exit(-1);
    }
    strncpy(setcol, token, strlen(token) - 1);
    setcol[strlen(token) - 1] = '\0';

    token = strtok(NULL, " ");
    if (token == NULL)
    {
        exit(-1);
    }
    strncpy(setval, token, strlen(token));
    setval[strlen(token)] = '\0';

    token = strtok(NULL, " ");
    if (token == NULL || strcmp(token, "WHERE") != 0)
    {
        exit(-1);
    }

    token = strtok(NULL, " ");
    if (token == NULL)
    {
        exit(-1);
    }
    strncpy(wherecol, token, strlen(token));
    wherecol[strlen(token)] = '\0';

    char *operators[] = {"=", "!=", "<", ">", "<=", ">="};
    int foundOperator = 0;
    token = strtok(NULL, " ");
    for (int i = 0; i < 6; i++)
    {
        if (strcmp(token, operators[i]) == 0)
        {
            strncpy(whereop, operators[i], strlen(token));
            whereop[strlen(token)] = '\0';
            foundOperator = 1;
            break;
        }
    }
    if (!foundOperator)
    {
        exit(-1);
    }

    token = strtok(NULL, " ");
    if (token == NULL)
    {
        exit(-1);
    }
    strncpy(whereval, token, strlen(token));
    whereval[strlen(token)] = '\0';

    // printf("UPDATE command parsed:\n");
    // printf("Table: %s\n", tableName);
    // printf("SET: %s = %s\n", setcol, setval);
    // printf("WHERE: %s %s %s\n", wherecol, whereop, whereval);
}

void append_to_file(const char *filename, const char *text)
{
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

void creareFisier(const char *filename)
{
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        perror("Eroare la crearea fișierului");
        return 1;
    }
    if (close(fd) == -1)
    {
        perror("Eroare la închiderea fișierului");
        return 1;
    }
}

Table *parseCreateTable(SQLParser *parser, char *stream, int socket)
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
    while ((token = strtok(NULL, " ,();")) != NULL)
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
            if (token != NULL)
            {
                int x = atoi(token);
                c[numarColoane].varchar_length = x;
            }
            else
            {
                printf("Eroare: Token este NULL\n");
            }
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
    pthread_mutex_lock(&mutex);
    t->numarColoane = numarColoane;
    send(socket, numeTabel, strlen(numeTabel), 0);
    printf("Tabel creat: %s\n", numeTabel);
    for (int i = 0; i < numarColoane; i++)
    {
        printf("Coloana %d: %s, Tip: ", i + 1, numeColoane[i]);
        char buffer_auxiliar[BUFSIZ];
        snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "Coloana %d: %s, Tip: \t%s", i + 1, numeColoane[i], tipuriDate[i]);
        send(socket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
    }
    pthread_mutex_unlock(&mutex);
    salveazaTabel(t, t->numeTabel);
    return t;
}

void parseDelete(SQLParser *parser, char *stream, char *tableName, char *wherecol, char *whereval, char *whereop, int socket)
{
    char *token = strtok(stream, " ");

    token = strtok(NULL, " ");
    if (token == NULL || strcmp(token, "FROM") != 0)
    {
        exit(-1);
    }

    token = strtok(NULL, " ");
    if (token == NULL)
    {
        exit(-1);
    }
    strncpy(tableName, token, strlen(token));
    tableName[strlen(token)] = '\0';

    if (!exists_in_master("master", tableName))
    {
        exit(-1);
    }

    token = strtok(NULL, " ");
    if (token == NULL || strcmp(token, "WHERE") != 0)
    {
        exit(-1);
    }

    token = strtok(NULL, " ");
    if (token == NULL)
    {
        exit(-1);
    }
    strncpy(wherecol, token, strlen(token));
    wherecol[strlen(token)] = '\0';

    char *operators[] = {"=", "!=", "<", ">", "<=", ">="};
    int foundOperator = 0;
    token = strtok(NULL, " ");
    for (int i = 0; i < 6; i++)
    {
        if (strcmp(token, operators[i]) == 0)
        {
            strncpy(whereop, operators[i], strlen(token));
            whereop[strlen(token)] = '\0';
            foundOperator = 1;
            break;
        }
    }
    if (!foundOperator)
    {
        exit(-1);
    }

    token = strtok(NULL, " ");
    if (token == NULL)
    {
        exit(-1);
    }
    strncpy(whereval, token, strlen(token));
    whereval[strlen(token)] = '\0';
    pthread_mutex_lock(&mutex);
    printf("Table: %s\n", tableName);
    printf("WHERE Column: %s\n", wherecol);
    printf("WHERE Operator: %s\n", whereop);
    printf("WHERE Value: %s\n", whereval);
    pthread_mutex_unlock(&mutex);
    char buffer[BUFSIZ];
    snprintf(buffer, sizeof(buffer), "Table: %s\n", tableName);
    send(socket, buffer, strlen(buffer), 0);

    snprintf(buffer, sizeof(buffer), "WHERE Column: %s\n", wherecol);
    send(socket, buffer, strlen(buffer), 0);

    snprintf(buffer, sizeof(buffer), "WHERE Operator: %s\n", whereop);
    send(socket, buffer, strlen(buffer), 0);

    snprintf(buffer, sizeof(buffer), "WHERE Value: %s\n", whereval);
    send(socket, buffer, strlen(buffer), 0);
}