#define _XOPEN_SOURCE 700
#include <pthread.h>
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

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void afiseazaTabel(Table *tabel, int clientSocket)
{
    // pthread_mutex_lock(&print_mutex);
    printf("\033[32m-----------------------------------------------------------\033[0m\n");
    send(clientSocket, "\033[32m-----------------------------------------------------------\033[0m\n",
         strlen("\033[32m-----------------------------------------------------------\033[0m\n"), 0);
    send(clientSocket, "\033[32m| \033[0m", strlen("\033[32m| \033[0m"), 0);
    printf("\033[32m| \033[0m");

    for (int i = 0; i < tabel->numarColoane; i++)
    {
        printf("\033[31m%s\t|\033[0m", tabel->coloane[i].numeColoana);

        char coloredColumn[1024];
        snprintf(coloredColumn, sizeof(coloredColumn), "\033[33m%s\033[0m", tabel->coloane[i].numeColoana);
        send(clientSocket, coloredColumn, strlen(coloredColumn), 0);
        send(clientSocket, "\033[33m\t|\033[0m", strlen("\033[33m\t|\033[0m"), 0);
    }
    printf("\033[32m\n-----------------------------------------------------------\n\033[0m");
    send(clientSocket, "\033[32m\n-----------------------------------------------------------\n\033[0m",
         strlen("\033[32m\n-----------------------------------------------------------\n\033[0m"), 0);

    // pthread_mutex_unlock(&print_mutex);
    // send(clientSocket,"SUNT AICI", strlen("SUNT AICI"), 0);
    for (int i = 0; i < tabel->numarRanduri; i++)
    {
        printf("\033[32m| \033[0m");
        for (int j = 0; j < tabel->numarColoane; j++)
        {
            // pthread_mutex_lock(&print_mutex);
            printf("\033[34m %s\t|\033[0m", tabel->randuri[i].elemente[j]);

            char coloredValue[1024];
            snprintf(coloredValue, sizeof(coloredValue), "\033[34m%s\033[0m", tabel->randuri[i].elemente[j]);
            send(clientSocket, coloredValue, strlen(coloredValue), 0);
            send(clientSocket, "\033[34m\t|\033[0m", strlen("\033[34m\t|\033[0m"), 0);
        }
        send(clientSocket, "\033[32m\n-----------------------------------------------------------\n\033[0m",
             strlen("\033[32m\n-----------------------------------------------------------\n\033[0m"), 0);
        printf("\033[32m\n-----------------------------------------------------------\n\033[0m");
    }
    // pthread_mutex_unlock(&print_mutex);
}

void afisare_nice(int clientSocket, Table *tabel, char **coloane, int nrcols, int *colindexes, BSTNode **searched, int nrfound)
{
    printf("\033[32m-----------------------------------------------------------\n\033[0m");
    send(clientSocket, "\033[32m-----------------------------------------------------------\n\033[0m",
         strlen("\033[32m-----------------------------------------------------------\n\033[0m"), 0);

    send(clientSocket, "\033[32m| \033[0m", strlen("\033[32m| \033[0m"), 0);
    printf("\033[32m| \033[0m");

    for (int i = 0; i < nrcols; i++)
    {
        printf("\033[33m%s\t|\033[0m", coloane[i]);

        char coloredColumn[1024];
        snprintf(coloredColumn, sizeof(coloredColumn), "\033[33m%s\t|\033[0m", coloane[i]);
        send(clientSocket, coloredColumn, strlen(coloredColumn), 0);
    }

    printf("\033[32m\n-----------------------------------------------------------\n\033[0m");
    send(clientSocket, "\033[32m\n-----------------------------------------------------------\n\033[0m",
         strlen("\033[32m\n-----------------------------------------------------------\n\033[0m"), 0);

    send(clientSocket, "\033[32m| \033[0m", strlen("\033[32m| \033[0m"), 0);
    printf("\033[32m| \033[0m");

    for (int i = 0; i < nrfound; i++)
    {
        for (int j = 0; j < nrcols; j++)
        {
            int row = searched[i]->row;
            printf("\033[34m%s\t|\033[0m", tabel->randuri[row].elemente[colindexes[j]]);

            char coloredValue[1024];
            snprintf(coloredValue, sizeof(coloredValue), "\033[34m%s\t|\033[0m", tabel->randuri[row].elemente[colindexes[j]]);
            send(clientSocket, coloredValue, strlen(coloredValue), 0);
        }
        printf("\033[32m\n-----------------------------------------------------------\n\033[0m");
        send(clientSocket, "\033[32m\n-----------------------------------------------------------\n\033[0m",
             strlen("\033[32m\n-----------------------------------------------------------\n\033[0m"), 0);

        send(clientSocket, "\033[32m| \033[0m", strlen("\033[32m| \033[0m"), 0);
        if (nrfound - i > 1)
            printf("\033[32m| \033[0m");
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

Table *creazaTabel(const char *numeTabel, Column *coloane, int numarColoane, int socketNumber)
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
        if (strcmp(tabel->coloane[i].tipDate, "VARCHAR") == 0)
        {
            char varcharLengthStr[20];
            snprintf(varcharLengthStr, sizeof(varcharLengthStr), "(%d)", tabel->coloane[i].varchar_length);
            fileSize += strlen("VARCHAR") + strlen(varcharLengthStr) + 1;
        }
        else
        {
            fileSize += strlen(tabel->coloane[i].tipDate) + 1;
        }
    }
    fileSize += 1;
    for (int i = 0; i < tabel->numarRanduri; i++)
    {
        for (int j = 0; j < tabel->numarColoane; j++)
        {
            fileSize += strlen(tabel->randuri[i].elemente[j]) + 1;
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

        if (strcmp(tabel->coloane[i].tipDate, "VARCHAR") == 0)
        {
            const char *tip = "VARCHAR";
            len = strlen(tip);
            memcpy(ptr, tip, len);
            ptr += len;

            char varcharLengthStr[20];
            snprintf(varcharLengthStr, sizeof(varcharLengthStr), "(%d)", tabel->coloane[i].varchar_length);
            len = strlen(varcharLengthStr);
            memcpy(ptr, varcharLengthStr, len);
            ptr += len;
        }
        else
        {
            const char *tip = tabel->coloane[i].tipDate;
            len = strlen(tip);
            memcpy(ptr, tip, len);
            ptr += len;
        }
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
                        token = strtok(NULL, "\t");
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
        perror("Eroare la deschiderea fișierului pentru scriere");
        return;
    }

    // Scrierea numelui tabelului
    fprintf(fisier, "%s\n", tabel->numeTabel);

    // Scrierea numărului de coloane
    fprintf(fisier, "%d\n", tabel->numarColoane);

    // Scrierea detaliilor coloanelor
    for (int i = 0; i < tabel->numarColoane; i++)
    {
        if (strcmp(tabel->coloane[i].tipDate, "VARCHAR") == 0)
        {
            fprintf(fisier, "%s VARCHAR(%d)\t", tabel->coloane[i].numeColoana, tabel->coloane[i].varchar_length);
        }
        else
        {
            fprintf(fisier, "%s %s\t", tabel->coloane[i].numeColoana, tabel->coloane[i].tipDate);
        }
    }
    fprintf(fisier, "\n");

    // Scrierea rândurilor
    for (int i = 0; i < tabel->numarRanduri; i++)
    {
        for (int j = 0; j < tabel->numarColoane; j++)
        {
            fprintf(fisier, "%s\t", tabel->randuri[i].elemente[j]);
        }
        fprintf(fisier, "\n");
    }

    // Închiderea fișierului
    fclose(fisier);
}

void stergeRand(Table *tabel, int indexRand)
{
    if (indexRand < 0 || indexRand >= tabel->numarRanduri)
    {
        printf("Index invalid: %d\n", indexRand);
        return;
    }

    for (int i = 0; i < tabel->numarColoane; i++)
    {
        free(tabel->randuri[indexRand].elemente[i]);
    }
    free(tabel->randuri[indexRand].elemente);

    for (int i = indexRand; i < tabel->numarRanduri - 1; i++)
    {
        tabel->randuri[i] = tabel->randuri[i + 1];
        tabel->randuri[i].index = i;
    }

    tabel->numarRanduri--;

    tabel->randuri = realloc(tabel->randuri, tabel->numarRanduri * sizeof(Row));
    if (tabel->randuri == NULL && tabel->numarRanduri > 0)
    {
        perror("Eroare la realocarea memoriei pentru randuri");
        exit(EXIT_FAILURE);
    }
}