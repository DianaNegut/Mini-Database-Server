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
    char tipDate[10];
    int varchar_length; // optional pt varchar
} Column;

typedef struct
{
    char numeTabel[MAX_NAME_LENGTH];
    Column* coloane;
    int numarColoane;
    void *randuri[MAX_ROW];
    int numarRanduri; // trb actualizat cand se fac inserturi
} Table;

// Column creazaColoana(const char *numeCol, DataType type, int varchar_length)
// {
//     Column col;
//     strncpy(col.numeColoana, numeCol, MAX_NAME_LENGTH);
//     col.tipDate = type;
//     col.varchar_length = (type == VARCHAR) ? varchar_length : 0;
//     return col;
// }

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



// void salveazaTabel(Table *tabel, const char *filename)
// {
//     int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
//     if (file < 0) {
//         perror("Nu pot deschide fișierul");
//         return;
//     }
//     write(file, tabel->numeTabel, strlen(tabel->numeTabel));
//     write(file, "\n", 1);
//     char buffer[20];
//     snprintf(buffer, sizeof(buffer), "%d\n", tabel->numarColoane);
//     write(file, buffer, strlen(buffer));
//     for (int i = 0; i < tabel->numarColoane; i++) {
//         write(file, tabel->coloane[i].numeColoana, strlen(tabel->coloane[i].numeColoana));
//         write(file, " ", 1);
//         const char *tip;
//         if (tabel->coloane[i].tipDate == INT) {
//             tip = "INT";
//         } else if (tabel->coloane[i].tipDate == VARCHAR) {
//             tip = "VARCHAR";
//         } else if (tabel->coloane[i].tipDate == DATE) {
//             tip = "DATE";
//         } else {
//             tip = "UNKNOWN";
//         }
//         write(file, tip, strlen(tip));
//         write(file, "\n", 1); 
//     }

//     close(file);
// }

