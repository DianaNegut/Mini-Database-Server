#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include "gestionareTabele.h"

#define MAX_COLUMN 1024
#define MAX_ROW 2048
#define BUFFER_SIZE 1024
#define MAX_NAME_LENGTH 100

bool columnsEqual(Column *col1, Column *col2)
{
    return strcmp(col1->numeColoana, col2->numeColoana) == 0;
}

void addIndexColumn(Table *table, Column *col)
{
    if (table->indexCount < MAX_COLUMNS)
    {
        table->indexColumns[table->indexCount++] = col;
    }
}

bool checkIfColumnIndexed(Table *table, Column *column)
{
    for (int i = 0; i < table->indexCount; i++)
    {
        if (columnsEqual(table->indexColumns[i], column))
        {
            return true;
        }
    }
    return false;
}

void addColumn(Table *table, char *name, char *type, int length)
{
    Column col;
    strcpy(col.numeColoana, name);
    strcpy(col.tipDate, type);
    col.varchar_length = length;
    table->coloane[table->numarColoane++] = col;
}

Column *getColumnByName(Table *table, const char *name)
{
    for (int i = 0; i < table->numarColoane; i++)
    {
        if (strcmp(table->coloane[i].numeColoana, name) == 0)
        {
            return &table->coloane[i];
        }
    }
    return NULL;
}

Table *incarcareTabel(const char *filename);
int getColumnIndex(Table *table, Column *col)
{
    for (int i = 0; i < table->numarColoane; i++)
    {
        if (columnsEqual(&table->coloane[i], col))
        {
            return i;
        }
    }
    return -1;
}

Table *creazaTabel(const char *numeTabel, Column *coloane, int numarColoane)
{
    Table *tabel = (Table *)malloc(sizeof(Table));
    strncpy(tabel->numeTabel, numeTabel, MAX_NAME_LENGTH);
    tabel->numarColoane = numarColoane;
    tabel->numarRanduri = 0;

    for (int i = 0; i < numarColoane; i++)
    {
        tabel->coloane[i] = coloane[i];
    }

    printf("Tabelul '%s' a fost creat cu %d coloane.\n", tabel->numeTabel, tabel->numarColoane);
    return tabel;
}

int citesteString(int fd, char *s)
{
    char buffer[BUFFER_SIZE];
    int index = 0;
    while (1)
    {
        int bytesRead = read(fd, buffer, 1);
        if (bytesRead <= 0)
            break;

        if (buffer[0] == '\n')
        {
            s[index] = '\0';
            return 1;
        }

        s[index++] = buffer[0];
    }
    s[index] = '\0';
    return 0;
}

int citesteNumar(int fd, char *s, int *dim)
{
    char buffer[1];
    int index = 0;
    *dim = 0;

    while (1)
    {
        int bytesRead = read(fd, buffer, 1);
        if (bytesRead <= 0)
            break;

        if (buffer[0] == '\n' || buffer[0] == ' ')
        {
            s[index] = '\0'; 
            *dim = index;
            return 1;
        }

        if (buffer[0] >= '0' && buffer[0] <= '9')
        {
            s[index++] = buffer[0];
        }
    }

    s[index] = '\0';
    *dim = index;

    return 0;
}


int citesteInt(int fd)
{
    char buffer[BUFFER_SIZE];
    int index = 0;
    int val;

    while (1)
    {
        int bytesRead = read(fd, &buffer[index], 1);
        if (bytesRead <= 0)
            break;

        if (buffer[index] == ' ' || buffer[index] == '\n')
        {
            buffer[index] = '\0';
            val = atoi(buffer);
            return val;
        }
        index++;
    }
    return 0;
}

void salveazaTabel(Table *tabel, const char *filename)
{
    int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file < 0)
    {
        perror("Nu pot deschide fișierul");
        return;
    }
    write(file, tabel->numeTabel, strlen(tabel->numeTabel));
    write(file, "\n", 1);
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%d\n", tabel->numarColoane);
    write(file, buffer, strlen(buffer));
    for (int i = 0; i < tabel->numarColoane; i++)
    {
        write(file, tabel->coloane[i].numeColoana, strlen(tabel->coloane[i].numeColoana));
        write(file, " ", 1);
        const char *tip;
        if (strcmp(tabel->coloane[i].tipDate, "INT"))
        {
            tip = "INT";
        }
        else if (strcmp(tabel->coloane[i].tipDate, "VARCHAR"))
        {
            tip = "VARCHAR";
        }
        else if (strcmp(tabel->coloane[i].tipDate, "DATE"))
        {
            tip = "DATE";
        }
        else
        {
            tip = "UNKNOWN";
        }
        write(file, tip, strlen(tip));
        write(file, "\t", 1);
        write(file, "\n", 1);
    }

    close(file);
}

Table* loadTable(const char* filename) {
    FILE* fd = fopen(filename, "r");
    if (fd == NULL) {
        perror("Eroare la deschiderea fișierului");
        return NULL;
    }

    Table* table = (Table*)malloc(sizeof(Table));
    if (!table) {
        perror("Eroare la alocarea memoriei pentru tabel");
        fclose(fd);
        return NULL;
    }
    memset(table, 0, sizeof(Table));

    char buffer[BUFFER_SIZE];
    int lineNumber = 0;

    while (fgets(buffer, BUFFER_SIZE, fd)) {
        buffer[strcspn(buffer, "\n")] = '\0';  

        if (lineNumber == 0) {
            strncpy(table->numeTabel, buffer, MAX_STRING_LENGTH - 1);
            table->numeTabel[MAX_STRING_LENGTH - 1] = '\0';
        }
        else if (lineNumber == 1) {
            // A doua linie: numărul de coloane
            table->numarColoane = atoi(buffer);
            table->coloane = (Column*)malloc(table->numarColoane * sizeof(Column));
            if (!table->coloane) {
                perror("Eroare la alocarea memoriei pentru coloane");
                free(table);
                fclose(fd);
                return NULL;
            }
        }
        else if (lineNumber == 2) {
            char* token;
            int colIndex = 0;
            token = strtok(buffer, " \t");
            while (token != NULL && colIndex < table->numarColoane) {
                strncpy(table->coloane[colIndex].numeColoana, token, MAX_STRING_LENGTH - 1);
                table->coloane[colIndex].numeColoana[MAX_STRING_LENGTH - 1] = '\0';

                token = strtok(NULL, " \t");
                if (token) {
                    strncpy(table->coloane[colIndex].tipDate, token, sizeof(table->coloane[colIndex].tipDate) - 1);
                    table->coloane[colIndex].tipDate[sizeof(table->coloane[colIndex].tipDate) - 1] = '\0';
                }

                if (strcmp(table->coloane[colIndex].tipDate, "VARCHAR") == 0) {
                    table->coloane[colIndex].varchar_length = 50;
                }
                else {
                    table->coloane[colIndex].varchar_length = 0;
                }
                colIndex++;
                token = strtok(NULL, " \t");
            }
        }
        else {
            table->numarRanduri = countLinesInFile(table->numeTabel) - 3;

            Row* newRow = (Row*)malloc(sizeof(Row) * table->numarRanduri);
            for ( int i= 0; i < table->numarRanduri;i++)
                newRow[i].elemente = (char*)malloc(table->numarRanduri * table->numarColoane * sizeof(char));

            char* token = strtok(buffer, " \t");
            for (int i = 0; i < table->numarRanduri ; i++) {
                for (int j = 0; j < table->numarColoane; j++)
                {
                    if (token) {
                        newRow[i].elemente[j] = strdup(token);
                        token = strtok(NULL, " \t");
                    }
                    else {
                        newRow[i].elemente[j] = strdup("");
                    }
                }
                fgets(buffer, BUFFER_SIZE, fd);
                buffer[strcspn(buffer, "\n")] = '\0';
                token = strtok(buffer, " \t");

               
            }
            table->randuri = newRow;
            
        }

        lineNumber++;
    }

    fclose(fd);
    return table;
}

// Table *incarcareTabel(const char *filename)
// {
//     int fd = open(filename, O_RDONLY);
//     if (fd == -1)
//     {
//         perror("Eroare la deschiderea fișierului");
//         return 1;
//     }

//     Table *tabel = (Table *)malloc(sizeof(Table));
//     citesteString(fd, tabel->numeTabel);

//     tabel->numarColoane = citesteInt(fd);

//     tabel->coloane = (Column *)malloc(tabel->numarColoane * sizeof(Column));

//     for (int i = 0; i < tabel->numarColoane; i++)
//     {
//         citesteString(fd, tabel->coloane[i].numeColoana);
//         citesteString(fd, tabel->coloane[i].tipDate);
//         if (strcmp(tabel->coloane[i].tipDate, "VARCHAR") == 0)
//         {
//             tabel->coloane[i].varchar_length = citesteInt(fd);
//         }
//         else
//         {
//             tabel->coloane[i].varchar_length = 0;
//         }
//     }

//     tabel->numarRanduri = citesteInt(fd);
//     tabel->randuri = (void **)malloc(tabel->numarRanduri * sizeof(void *));

//     for (int i = 0; i < tabel->numarRanduri; i++)
//     {
//         tabel->randuri[i] = malloc(tabel->numarColoane * sizeof(void *));
//         for (int j = 0; j < tabel->numarColoane; j++)
//         {
//             if (strcmp(tabel->coloane[j].tipDate, "VARCHAR") == 0)
//             {
//                 char *val = (char *)malloc(tabel->coloane[j].varchar_length * sizeof(char));
//                 citesteString(fd, val);
//                 ((char **)tabel->randuri)[i * tabel->numarColoane + j] = val;
//             }
//             else
//             {
//                 int *val = (int *)malloc(sizeof(int));
//                 *val = citesteInt(fd);
//                 ((int **)tabel->randuri)[i * tabel->numarColoane + j] = val;
//             }
//         }
//     }

//     return tabel;
// }

char** getElemByColumn(Table* tabel, char* numeColoana, int*colIndex){
    *colIndex = -1;
    for (int i = 0; i < tabel->numarColoane; i++) {
        if (strcmp(tabel->coloane[i].numeColoana, numeColoana) == 0) {
            *colIndex = i;
            break;
        }
    }

    if (*colIndex == -1) {
        printf("Coloana %s nu a fost găsită.\n", numeColoana);
        return NULL;
    }

    char** valori = (char*)malloc(tabel->numarRanduri * sizeof(char));
    if (valori == NULL) {
        perror("Eroare la alocarea memoriei");
        return NULL;
    }

    for (int i = 0; i < tabel->numarRanduri; i++) {
        valori[i] = strdup(tabel->randuri[i].elemente[*colIndex]);
        if (valori[i] == NULL) {
            perror("Eroare la duplicarea șirului");
            for (int j = 0; j < i; j++) {
                free(valori[j]);
            }
            free(valori);
            return NULL;
        }
    }

    return valori;
}

int countLinesInFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Eroare la deschiderea fișierului");
        return -1;
    }

    int lines = 0;
    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        lines++;
    }

    fclose(file);
    return lines;
}