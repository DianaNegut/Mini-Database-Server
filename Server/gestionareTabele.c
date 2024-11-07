#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>


#define MAX_COLUMN 1024
#define MAX_ROW 2048
#define MAX_NAME_LENGTH 100

typedef enum
{
    INT,
    VARCHAR,
    DATE
} DataType;

typedef struct Column
{
    char numeColoana[MAX_NAME_LENGTH];
    DataType tipDate;
    int varchar_length; // optional pt varchar
} Column;

typedef struct
{
    char numeTabel[MAX_NAME_LENGTH];
    Column coloane[MAX_COLUMN];
    int numarColoane;
    void *randuri[MAX_ROW];
    int numarRanduri; // trb actualizat cand se fac inserturi
} Table;

Column creazaColoana(const char *numeCol, DataType type, int varchar_length)
{
    Column col;
    strncpy(col.numeColoana, numeCol, MAX_NAME_LENGTH);
    col.tipDate = type;
    col.varchar_length = (type == VARCHAR) ? varchar_length : 0;
    return col;
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

int insereazaRand(Table *tabel, void **valori)
{
    if (tabel->numarRanduri >= MAX_ROW)
    {
        printf("Tabelul '%s' este plin.\n", tabel->numeTabel);
        return -1;
    }
    tabel->randuri[tabel->numarRanduri] = malloc(tabel->numarColoane * sizeof(void *));

    for (int i = 0; i < tabel->numarColoane; i++)
    {
        DataType type = tabel->coloane[i].tipDate;

        if (type == INT)
        {
            int *intValue = malloc(sizeof(int));
            *intValue = *((int *)valori[i]);
            ((void **)tabel->randuri[tabel->numarRanduri])[i] = intValue;
        }
        else if (type == VARCHAR)
        {
            int length = tabel->coloane[i].varchar_length;
            char *varcharValue = malloc(length + 1);
            strncpy(varcharValue, (char *)valori[i], length);
            varcharValue[length] = '\0';
            ((void **)tabel->randuri[tabel->numarRanduri])[i] = varcharValue;
        }
        else if (type == DATE)
        {
            struct tm *dateValue = malloc(sizeof(struct tm));
            memcpy(dateValue, valori[i], sizeof(struct tm));
            ((void **)tabel->randuri[tabel->numarRanduri])[i] = dateValue;
        }
    }

    tabel->numarRanduri++;
    return 0;
}

void afiseazaTabel(Table *tabel)
{
    printf("Tabelul: %s\n", tabel->numeTabel);

    for (int i = 0; i < tabel->numarColoane; i++)
    {
        printf("%s\t", tabel->coloane[i].numeColoana);
    }
    printf("\n");

    for (int i = 0; i < tabel->numarRanduri; i++)
    {
        void **rand = (void **)tabel->randuri[i];

        for (int j = 0; j < tabel->numarColoane; j++)
        {
            DataType type = tabel->coloane[j].tipDate;

            if (type == INT)
            {
                printf("%d\t", *((int *)rand[j]));
            }
            else if (type == VARCHAR)
            {
                printf("%s\t", (char *)rand[j]);
            }
            else if (type == DATE)
            {
                struct tm *date = (struct tm *)rand[j];
                printf("%02d-%02d-%04d\t", date->tm_mday, date->tm_mon + 1, date->tm_year + 1900);
            }
        }
        printf("\n");
    }
}

void salveazaTabel(Table *tabel, const char *filename)
{
    int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file < 0) {
        perror("Nu pot deschide fișierul");
        return;
    }
    write(file, tabel->numeTabel, strlen(tabel->numeTabel));
    write(file, "\n", 1);
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%d\n", tabel->numarColoane);
    write(file, buffer, strlen(buffer));
    for (int i = 0; i < tabel->numarColoane; i++) {
        write(file, tabel->coloane[i].numeColoana, strlen(tabel->coloane[i].numeColoana));
        write(file, " ", 1);
        const char *tip;
        if (tabel->coloane[i].tipDate == INT) {
            tip = "INT";
        } else if (tabel->coloane[i].tipDate == VARCHAR) {
            tip = "VARCHAR";
        } else if (tabel->coloane[i].tipDate == DATE) {
            tip = "DATE";
        } else {
            tip = "UNKNOWN";
        }
        write(file, tip, strlen(tip));
        write(file, "\n", 1); 
    }

    close(file);
}

