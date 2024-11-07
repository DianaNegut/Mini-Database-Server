#ifndef GESTIONARE_TABEL_H
#define GESTIONARE_TABEL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef enum {
    INT,
    VARCHAR,
    DATE
} DataType;

typedef struct Column {
    char numeColoana[100];
    DataType tipDate;
    int varchar_length;
} Column;

typedef struct {
    char numeTabel[100];
    Column coloane[1024];
    int numarColoane ;
    void** randuri;  
    int numarRanduri ;
} Table;

Column* creazaColoana(const char* numeCol, DataType type, int varchar_length);
Table* creazaTabel(const char* numeTabel, Column* coloane, int numarColoane);
void salveazaTabel(Table* tabel, const char* filename);
void insereazaRand(Table* tabel, void** rand);
void afiseazaTabel(Table* tabel);

#endif // GESTIONARE_TABEL_H
