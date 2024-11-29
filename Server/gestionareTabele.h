#ifndef GESTIONARE_TABEL_H
#define GESTIONARE_TABEL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_COLUMNS 100
#define MAX_STRING_LENGTH 100


typedef struct Column {
    char numeColoana[MAX_STRING_LENGTH];
    char tipDate[10];
    int varchar_length;
} Column;

typedef struct Row{
    int index;
    char** elemente;
}Row;

typedef struct {
    char numeTabel[MAX_STRING_LENGTH];
    Column* coloane;
    Column* indexColumns[MAX_COLUMNS];
    Row* randuri;
    int indexCount;
    int numarColoane;
    int numarRanduri ;
} Table;


Table* loadTable(const char *filename);
Table* creazaTabel(const char* numeTabel, Column* coloane, int numarColoane);
void salveazaTabel(Table* tabel, const char* filename);
void insereazaRand(Table* tabel, void** rand);
void afiseazaTabel(Table* tabel);
bool columnsEqual (Column* col1, Column* col2);
void addColumn(Table* table, char* name, char* type, int length);
void addIndexColumn(Table* table, Column* col);
bool checkIfColumnIndexed(Table* table, Column* column);
int getColumnIndex(Table* table, Column* col) ;
Column* getColumnByName(Table* table, const char* name);
Table* incarcareTabel(const char* filename);
char** getElemByColumn(Table* tabel, char* numeColoana, int* colIndex);
int countLinesInFile(const char* filename);
void scrieTabelInFisier(const char *numeFisier, Table *tabel);


#endif // GESTIONARE_TABEL_H
