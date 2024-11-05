#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_COLUMN 1024
#define MAX_ROW 2048
#define MAX_NAME_LENGTH 100



typedef enum {
    INT,
    VARCHAR,
    DATE
} DataType;

typedef struct Column
{
    char numeColoana[MAX_NAME_LENGTH];
    DataType tipDate;
    int varchar_length; //optional pt varchar
}Column;


typedef struct {
    char numeTabel[MAX_NAME_LENGTH];
    Column coloane[MAX_COLUMN];
    int numarColoane;
    void* randuri[MAX_ROW];  
    int numarRanduri; //trb actualizat cand se fac inserturi
} Table;



Column creazaColoana(const char* numeCol, DataType type, int varchar_length) {
    Column col;
    strncpy(col.numeColoana, numeCol, MAX_NAME_LENGTH);
    col.tipDate = type;
    col.varchar_length = (type == VARCHAR) ? varchar_length : 0;
    return col;
}


Table* creazaTabel(const char* numeTabel, Column* coloane, int numarColoane) {
    Table* tabel = (Table*)malloc(sizeof(Table));
    strncpy(tabel->numeTabel, numeTabel, MAX_NAME_LENGTH);
    tabel->numarColoane = numarColoane;
    tabel->numarRanduri = 0;

    for (int i = 0; i < numarColoane; i++) {
        tabel->coloane[i] = coloane[i];
    }

    printf("Tabelul '%s' a fost creat cu %d coloane.\n", tabel->numeTabel, tabel->numarColoane);
    return tabel;
}


int insereazaRand(Table* tabel, void** valori) {
    if (tabel->numarRanduri >= MAX_ROW) {
        printf("Tabelul '%s' este plin.\n", tabel->numeTabel);
        return -1;
    }
    tabel->randuri[tabel->numarRanduri] = malloc(tabel->numarColoane * sizeof(void*));

    for (int i = 0; i < tabel->numarColoane; i++) {
        DataType type = tabel->coloane[i].tipDate;

        if (type == INT) {
            int* intValue = malloc(sizeof(int));
            *intValue = *((int*)valori[i]);
            ((void**)tabel->randuri[tabel->numarRanduri])[i] = intValue;
            
        } else if (type == VARCHAR) {
            int length = tabel->coloane[i].varchar_length;
            char* varcharValue = malloc(length + 1);
            strncpy(varcharValue, (char*)valori[i], length);
            varcharValue[length] = '\0';
            ((void**)tabel->randuri[tabel->numarRanduri])[i] = varcharValue;
            
        } else if (type == DATE) {
            struct tm* dateValue = malloc(sizeof(struct tm));
            memcpy(dateValue, valori[i], sizeof(struct tm));
            ((void**)tabel->randuri[tabel->numarRanduri])[i] = dateValue;
        }
    }

    tabel->numarRanduri++;
    return 0;
}




void afiseazaTabel(Table* tabel) {
    printf("Tabelul: %s\n", tabel->numeTabel);

    for (int i = 0; i < tabel->numarColoane; i++) {
        printf("%s\t", tabel->coloane[i].numeColoana);
    }
    printf("\n");

    for (int i = 0; i < tabel->numarRanduri; i++) {
        void** rand = (void**)tabel->randuri[i];  

        for (int j = 0; j < tabel->numarColoane; j++) {
            DataType type = tabel->coloane[j].tipDate;

            if (type == INT) {
                printf("%d\t", *((int*)rand[j]));
            } else if (type == VARCHAR) {
                printf("%s\t", (char*)rand[j]);
            } else if (type == DATE) {
                struct tm* date = (struct tm*)rand[j];
                printf("%02d-%02d-%04d\t", date->tm_mday, date->tm_mon + 1, date->tm_year + 1900);
            }
        }
        printf("\n");
    }
}




void salveazaTabel(Table* tabel, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Nu pot deschide fișierul");
        return;
    }
    fwrite(tabel->numeTabel, sizeof(char), MAX_NAME_LENGTH, file);
    fwrite(&tabel->numarColoane, sizeof(int), 1, file);
    for (int i = 0; i < tabel->numarColoane; i++) {
        fwrite(tabel->coloane[i].numeColoana, sizeof(char), MAX_NAME_LENGTH, file);
       // fwrite(&tabel->coloane[i].tipDate, sizeof(DataType), 1, file);
       // fwrite(&tabel->coloane[i].varchar_length, sizeof(int), 1, file);
    }
    fwrite(&tabel->numarRanduri, sizeof(int), 1, file);
    for (int i = 0; i < tabel->numarRanduri; i++) {
        void** rand = (void**)tabel->randuri[i]; 

        for (int j = 0; j < tabel->numarColoane; j++) {
            DataType type = tabel->coloane[j].tipDate;

            if (type == INT) {
                fwrite(rand[j], sizeof(int), 1, file);
            } else if (type == VARCHAR) {
                int len = strlen((char*)rand[j]) + 1;
                fwrite(&len, sizeof(int), 1, file);
                fwrite(rand[j], sizeof(char), len, file);
            } else if (type == DATE) {
                fwrite(rand[j], sizeof(struct tm), 1, file);
            }
        }
    }
    fclose(file);
}


