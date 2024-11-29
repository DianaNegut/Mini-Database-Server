#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
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

void afiseazaTabel(Table *tabel)
{
    printf("-----------------------------------------------------------\n");
    printf("| ");
    for (int i = 0; i < tabel->numarColoane; i++)
    {
        printf("%s\t|", tabel->coloane[i].numeColoana);
    }
    printf("\n-----------------------------------------------------------\n");
    for (int i = 0; i < tabel->numarRanduri; i++)
    {
        printf("| ");
        for (int j = 0; j < tabel->numarColoane; j++)
        {
            printf(" %s\t|", tabel->randuri[i].elemente[j]);
        }
        printf("\n-----------------------------------------------------------\n");
    }
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
    int file = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (file < 0)
    {
        perror("Nu pot deschide fișierul!\n");
        return;
    }

    size_t fileSize = strlen(tabel->numeTabel) + 1;
    for (int i = 0; i < tabel->numarColoane; i++)
    {
        fileSize += strlen(tabel->coloane[i].numeColoana) + 1;
        fileSize += strlen(tabel->coloane[i].tipDate) + 1;
    }
    fileSize += 1;
    for (int i = 0; i < tabel->numarRanduri; i++)
    {
        for (int j = 0; j < tabel->numarColoane; j++)
        {
            fileSize += strlen(tabel->randuri[i].elemente[j]) + 1;
            fileSize += 1;
        }
        fileSize += 1;
    }

    if (ftruncate(file, fileSize) < 0)
    {
        perror("Nu pot seta dimensiunea fișierului");
        close(file);
        return;
    }

    char *map = mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
    if (map == MAP_FAILED)
    {
        perror("Nu pot mapa fișierul");
        close(file);
        return;
    }

    char *ptr = map;

    size_t len = strlen(tabel->numeTabel);
    memcpy(ptr, tabel->numeTabel, len);
    ptr += len;
    *ptr++ = '\n';

    len = snprintf(ptr, 20, "%d\n", tabel->numarColoane);
    ptr += len;

    for (int i = 0; i < tabel->numarColoane; i++)
    {
        len = strlen(tabel->coloane[i].numeColoana);
        memcpy(ptr, tabel->coloane[i].numeColoana, len);
        ptr += len;
        *ptr++ = ' ';

        const char *tip;
        if (strcmp(tabel->coloane[i].tipDate, "INT") == 0)
            tip = "INT";
        else if (strcmp(tabel->coloane[i].tipDate, "VARCHAR") == 0)
        {
            tip = "VARCHAR";

        }
        else if (strcmp(tabel->coloane[i].tipDate, "DATE") == 0)
            tip = "DATE";
        else
            tip = "UNKNOWN";

        len = strlen(tip);
        memcpy(ptr, tip, len);
        ptr += len;
        *ptr++ = '\t';
    }
    *ptr++ = '\n';
    for (int i = 0; i < tabel->numarRanduri; i++)
    {
        for (int j = 0; j < tabel->numarColoane; j++)
        {
            len = strlen(tabel->randuri[i].elemente[j]);
            memcpy(ptr, tabel->randuri[i].elemente[j], len);
            ptr += len;
            *ptr++ = '\t';
        }
        *ptr++ = '\n';
    }
    //*ptr++ = '\0';

    if (msync(map, fileSize, MS_SYNC) < 0)
    {
        perror("Eroare la sincronizarea datelor");
    }

    if (munmap(map, fileSize) < 0)
    {
        perror("Eroare la unmap");
    }

    close(file);
}

Table *loadTable(const char *filename)
{
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("Eroare la deschiderea fișierului");
        return NULL;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        perror("Eroare la fstat");
        close(fd);
        return NULL;
    }

    size_t fileSize = sb.st_size;
    char *fileContent = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (fileContent == MAP_FAILED)
    {
        perror("Eroare la mmap");
        close(fd);
        return NULL;
    }

    Table *table = (Table *)malloc(sizeof(Table));
    if (!table)
    {
        perror("Eroare la alocarea memoriei pentru tabel");
        munmap(fileContent, fileSize);
        close(fd);
        return NULL;
    }
    memset(table, 0, sizeof(Table));

    const char *ptr = fileContent;
    const char *end = fileContent + fileSize;
    int lineNumber = 0;

    while (ptr < end)
    {

        const char *lineStart = ptr;
        while (ptr < end && *ptr != '\n')
            ptr++;
        size_t lineLength = ptr - lineStart;
        char buffer[BUFFER_SIZE] = {0};
        strncpy(buffer, lineStart, lineLength);

        if (ptr < end && *ptr == '\n')
            ptr++;

        buffer[strcspn(buffer, "\n")] = '\0';

        if (lineNumber == 0)
        {
            strncpy(table->numeTabel, buffer, MAX_STRING_LENGTH - 1);
            table->numeTabel[MAX_STRING_LENGTH - 1] = '\0';
        }
        else if (lineNumber == 1)
        {
            table->numarColoane = atoi(buffer);
            table->coloane = (Column *)malloc(table->numarColoane * sizeof(Column));
            if (!table->coloane)
            {
                perror("Eroare la alocarea memoriei pentru coloane");
                free(table);
                munmap(fileContent, fileSize);
                close(fd);
                return NULL;
            }
        }
        else if (lineNumber == 2)
        {
            char *token;
            int colIndex = 0;
            token = strtok(buffer, " \t");
            while (token != NULL && colIndex < table->numarColoane)
            {
                strncpy(table->coloane[colIndex].numeColoana, token, MAX_STRING_LENGTH - 1);
                table->coloane[colIndex].numeColoana[MAX_STRING_LENGTH - 1] = '\0';

                token = strtok(NULL, " \t");
                if (token && strstr(token, "VARCHAR") == 0)
                {
                    strncpy(table->coloane[colIndex].tipDate, token, sizeof(table->coloane[colIndex].tipDate) - 1);
                    table->coloane[colIndex].tipDate[sizeof(table->coloane[colIndex].tipDate) - 1] = '\0';

                    table->coloane[colIndex].varchar_length = 0;
                }
                else if (token)
                {
                    strncpy(table->coloane[colIndex].tipDate, token, 7);
                    table->coloane[colIndex].tipDate[7] = '\0';

                    char *nr = strdup(token + 8);
                    nr[strlen(nr) - 1] = '\0';

                    table->coloane[colIndex].varchar_length = atoi(nr);
                    free(nr);
                }

                colIndex++;
                token = strtok(NULL, " \t");
            }
        }
        else
        {
            table->numarRanduri = countLinesInFile(table->numeTabel) - 3;

            Row *newRow = (Row *)malloc(sizeof(Row) * table->numarRanduri);
            for (int i = 0; i < table->numarRanduri; i++)
                newRow[i].elemente = (char **)malloc(table->numarColoane * sizeof(char *));

            char *token = strtok(buffer, " \t");
            for (int i = 0; i < table->numarRanduri; i++)
            {
                for (int j = 0; j < table->numarColoane; j++)
                {
                    if (token)
                    {
                        newRow[i].elemente[j] = strdup(token);
                        token = strtok(NULL, " \t");
                    }
                    else
                    {
                        printf("??????");
                    }
                }
                if (ptr < end)
                {
                    lineStart = ptr;
                    while (ptr < end && *ptr != '\n')
                        ptr++;
                    lineLength = ptr - lineStart;
                    strncpy(buffer, lineStart, lineLength);
                    buffer[lineLength] = '\0';
                    if (ptr < end && *ptr == '\n')
                        ptr++;
                    token = strtok(buffer, " \t");
                }
            }
            table->randuri = newRow;
        }

        lineNumber++;
    }

    munmap(fileContent, fileSize);
    close(fd);
    return table;
}

char **getElemByColumn(Table *tabel, char *numeColoana, int *colIndex)
{
    *colIndex = -1;
    for (int i = 0; i < tabel->numarColoane; i++)
    {
        if (strcmp(tabel->coloane[i].numeColoana, numeColoana) == 0)
        {
            *colIndex = i;
            break;
        }
    }

    if (*colIndex == -1)
    {
        printf("Coloana %s nu a fost găsită.\n", numeColoana);
        return NULL;
    }

    char **valori = (char **)malloc(tabel->numarRanduri * sizeof(char *));
    if (valori == NULL)
    {
        perror("Eroare la alocarea memoriei");
        return NULL;
    }

    for (int i = 0; i < tabel->numarRanduri; i++)
    {
        valori[i] = strdup(tabel->randuri[i].elemente[*colIndex]);
        if (valori[i] == NULL)
        {
            perror("Eroare la duplicarea șirului");
            for (int j = 0; j < i; j++)
            {
                free(valori[j]);
            }
            free(valori);
            return NULL;
        }
    }

    return valori;
}

int countLinesInFile(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Eroare la deschiderea fișierului");
        return -1;
    }

    int lines = 0;
    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        lines++;
    }

    fclose(file);
    return lines;
}

void scrieTabelInFisier(const char *numeFisier, Table *tabel)
{
    FILE *fisier = fopen(numeFisier, "w");
    if (fisier == NULL)
    {
        perror("Eroare la deschiderea fișierului.");
        return;
    }

    fprintf(fisier, "%s\n", tabel->numeTabel);

    fprintf(fisier, "%d\n", tabel->numarColoane);

    for (int i = 0; i < tabel->numarColoane; i++)
    {
        fprintf(fisier, "%s %s", tabel->coloane[i].numeColoana, tabel->coloane[i].tipDate);
        if (strcmp(tabel->coloane[i].tipDate, "VARCHAR") == 0)
        {
            fprintf(fisier, "(%d)", tabel->coloane[i].varchar_length);
        }
        if (i < tabel->numarColoane - 1)
        {
            fprintf(fisier, "\t");
        }
    }
    fprintf(fisier, "\n");

    for (int i = 0; i < tabel->numarRanduri; i++)
    {
        for (int j = 0; j < tabel->numarColoane; j++)
        {
            fprintf(fisier, "%s\t", tabel->randuri[i].elemente[j]);
        }
        if (i < tabel->numarRanduri - 1)
            fprintf(fisier, "\n");
    }

    fclose(fisier);
    printf("Tabelul a fost actualizat în fișierul %s.\n", numeFisier);
}