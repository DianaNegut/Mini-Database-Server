
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "SQLParser.h"
#include <pthread.h>
#include "gestionareTabele.h"
#include "cache.h"
#include "threadPool.h"

#define PORT 8127
#define BUFFER_SIZE 1024
#define THREAD_COUNT 4
#define QUEUE_SIZE 10

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;
Cache *cache;

typedef struct ClientTask
{
    int clientSocket;
} ClientTask;

void handleClientWrapper(void *arg)
{
    ClientTask *task = (ClientTask *)arg;
    handleClient(task->clientSocket);
    free(task);
}

bool executeSelectTotal(int clientSocket, Table *tabel, char *nume_tabel, char *from_ptr)
{
    sscanf(from_ptr + 5, "%s", nume_tabel);
    char buffer_auxiliar[BUFFER_SIZE];
    pthread_mutex_lock(&display_mutex);
    snprintf(buffer_auxiliar, strlen(buffer_auxiliar), "Numele tabelului este: %s\n", nume_tabel);
    send(clientSocket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
    printf("Numele tabelului este: %s\n", nume_tabel);
    pthread_mutex_unlock(&display_mutex);
    tabel = loadTable(nume_tabel);
    pthread_mutex_lock(&display_mutex);
    afiseazaTabel(tabel, clientSocket);
    pthread_mutex_unlock(&display_mutex);
    return true;
}

bool executeSelectConditii(SQLParser *parser, char *buffer, char *titleTabel, Table *tabel, int clientSocket)
{
    char *wherecol = (char *)malloc(20);
    char *whereval = (char *)malloc(20);
    char *whereop = (char *)malloc(3 * sizeof(char));
    char **columns = parser->parseSelect(parser, buffer, titleTabel, wherecol, whereval, whereop);

    tabel = loadTable(titleTabel);

    int *colindexes = (int *)malloc(5 * sizeof(int));
    int nrselectedcols = 0;
    if (strcmp(columns[0], "*") != 0)
    {
        int i = 0;
        while (columns[i] != NULL)
        {
            Column *col = (Column *)malloc(sizeof(Column));
            col = getColumnByName(tabel, (const char *)columns[i]);
            colindexes[i] = getColumnIndex(tabel, col);
            nrselectedcols++;
            i++;
        }
    }
    else
    {
        nrselectedcols = tabel->numarColoane;
        for (int i = 0; i < nrselectedcols; i++)
        {
            colindexes[i] = i;
            columns[i] = strdup(tabel->coloane[i].numeColoana);
        }
    }
    if (strcmp(wherecol, "") == 0)
    {
        printf("NU AVEM CLAUZA WHERE\n");
        for (int i = 0; i < nrselectedcols; i++)
        {
            char **aux = getElemByColumn(tabel, columns[i], &colindexes[i]);
            for (int j = 0; j < tabel->numarRanduri; j++)
            {
                printf("%s ", aux[j]);
            }
            printf("\n");
        }
        free(wherecol);
        free(whereval);
        free(whereop);
        free(columns);
        free(colindexes);
        return true;
    }

    char **elemente = (char *)malloc(tabel->numarRanduri * sizeof(char));
    for (int j = 0; j < tabel->numarRanduri; j++)
        elemente[j] = (char *)malloc(10);
    int *colIndex = malloc(sizeof(int));
    elemente = getElemByColumn(tabel, wherecol, colIndex);

    int *rowIndex = malloc(sizeof(int));
    *rowIndex = 0;
    BSTNode *root = buildBST(elemente, tabel->numarRanduri, *colIndex, rowIndex);

    BSTNode **searched = (BSTNode *)malloc(10 * sizeof(BSTNode));
    int found = 0;

    if (strcmp(whereop, "=") == 0)
    {
        searched = findNodesWithValue(root, whereval, &found);
        pthread_mutex_lock(&display_mutex);
        afisare_nice(clientSocket, tabel, columns, nrselectedcols, colindexes, searched, found);
        pthread_mutex_unlock(&display_mutex);
    }
    else if (strcmp(whereop, "<=") == 0 || strcmp(whereop, ">=") == 0 || strcmp(whereop, "<") == 0 || strcmp(whereop, ">") == 0)
    {
        int n = 0;
        searched = getNodesByCondition(root, whereval, whereop, &n);
        pthread_mutex_lock(&display_mutex);
        afisare_nice(clientSocket, tabel, columns, nrselectedcols, colindexes, searched, n);
        pthread_mutex_unlock(&display_mutex);
    }
    else if (strcmp(whereop, "!=") == 0)
    {
        int n = 0;
        searched = getNodesExcluding(root, whereval, &n);
        pthread_mutex_lock(&display_mutex);
        afisare_nice(clientSocket, tabel, columns, nrselectedcols, colindexes, searched, n);
        pthread_mutex_unlock(&display_mutex);
    }
    else
    {
        pthread_mutex_lock(&display_mutex);
        printf("Operator necunoscut in clauza WHERE\n");
        send(clientSocket, "Operator necunoscut în clauza WHERE\n", strlen("Operator necunoscut în clauza WHERE\n"), 0);
        pthread_mutex_unlock(&display_mutex);
        return false;
    }
    free(wherecol);
    free(whereval);
    free(whereop);
    free(columns);
    free(colindexes);
    free(elemente);
    free(colIndex);
    free(rowIndex);
    free(searched);
    return true;
}

bool comportamentSelect(int clientSocket, char *buffer, Table *tabel, SQLParser *parser, char *titleTabel, char *result)
{
    char result_aux[MAX_LENGTH];
    char* result_aux_ptr;
    char *copie_buffer = strdup(buffer);
    char *token = strtok(buffer, "*");
    if (strcmp(token, "SELECT ") == 0)
    {
        // face SELECT * FROM nume tabel
        char nume_tabel[50];
        char *from_ptr;
        from_ptr = strstr(copie_buffer, "FROM");
        if (from_ptr != NULL)
        {
            return executeSelectTotal(clientSocket, tabel, nume_tabel, from_ptr);
        }
        else
        {
            strcpy(result_aux,"Nu s-a găsit un tabel în interogare.\n");
            result_aux_ptr=strdup(result_aux);
            pthread_mutex_lock(&display_mutex);
            send(clientSocket, "Nu s-a găsit un tabel în interogare.\n", strlen("Nu s-a găsit un tabel în interogare.\n"), 0);
            printf("Nu s-a găsit un tabel în interogare.\n");
            strcpy(result, result_aux_ptr);
            pthread_mutex_unlock(&display_mutex);
            return false;
        }
    }
    else
    {
        return executeSelectConditii(parser, buffer, titleTabel, tabel, clientSocket);
    }
    return true;
}

bool comportamentUpdate(SQLParser *parser, char *buffer, char *tableName, Table *tabel, int clientSocket)
{
    char *wherecol = (char *)malloc(20);
    char *whereval = (char *)malloc(20);
    char *whereop = (char *)malloc(20);
    char *setcol = (char *)malloc(20);
    char *setval = (char *)malloc(20);
    parseUpdate(parser, buffer, tableName, setcol, setval, wherecol, whereval, whereop);

    tabel = loadTable(tableName);

    char **elemente = (char **)malloc(tabel->numarRanduri * sizeof(char *)); // elementele din coloana din clauza WHERE
    for (int j = 0; j < tabel->numarRanduri; j++)
        elemente[j] = (char *)malloc(10);
    int *colIndex = malloc(sizeof(int)); // indexul coloanei din clauza WHERE
    elemente = getElemByColumn(tabel, wherecol, colIndex);

    int *rowIndex = malloc(sizeof(int));
    *rowIndex = 0;
    BSTNode *root = buildBST(elemente, tabel->numarRanduri, *colIndex, rowIndex);

    BSTNode **searched = (BSTNode **)malloc(10 * sizeof(BSTNode *));
    int found = 0;

    Column *col = malloc(sizeof(Column));
    col = getColumnByName(tabel, setcol);
    int setColIndex = getColumnIndex(tabel, col);

    if (strcmp(whereop, "=") == 0)
    {
        searched = findNodesWithValue(root, whereval, &found);
        printf("%d\n", found);
        for (int i = 0; i < found; i++)
        {
            int row = searched[i]->row;

            char buffer_auxiliar[BUFSIZ];
            snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "Din coloana %s schimbam elementul %s de pe randul %d\n", setcol, tabel->randuri[row].elemente[setColIndex], row + 1);
            send(clientSocket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
            printf("Din coloana %s schimbam elementul %s de pe randul %d\n", setcol, tabel->randuri[row].elemente[setColIndex], row + 1);
            tabel->randuri[row].elemente[setColIndex] = setval;
        }
        scrieTabelInFisier(tableName, tabel);
    }
    else if (strcmp(whereop, "<=") == 0 || strcmp(whereop, ">=") == 0 || strcmp(whereop, "<") == 0 || strcmp(whereop, ">") == 0)
    {
        int n = 0;
        searched = getNodesByCondition(root, whereval, whereop, &n);
        for (int i = 0; i < n; i++)
        {
            int row = searched[i]->row;
            char buffer_auxiliar[BUFSIZ];
            snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "Din coloana %s schimbam elementul %s de pe randul %d\n", setcol, tabel->randuri[row].elemente[setColIndex], row + 1);
            send(clientSocket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
            printf("Din coloana %s schimbam elementul %s de pe randul %d\n", setcol, tabel->randuri[row].elemente[setColIndex], row + 1);
            tabel->randuri[row].elemente[setColIndex] = setval;
        }
        scrieTabelInFisier(tableName, tabel);
    }
    else if (strcmp(whereop, "!=") == 0)
    {
        int n = 0;
        searched = getNodesExcluding(root, whereval, &n);
        for (int i = 0; i < n; i++)
        {
            int row = searched[i]->row;
            char buffer_auxiliar[BUFSIZ];
            snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "Din coloana %s schimbam elementul %s de pe randul %d\n", setcol, tabel->randuri[row].elemente[setColIndex], row + 1);
            send(clientSocket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
            printf("Din coloana %s schimbam elementul %s de pe randul %d\n", setcol, tabel->randuri[row].elemente[setColIndex], row + 1);
            tabel->randuri[row].elemente[setColIndex] = setval;
        }
        scrieTabelInFisier(tableName, tabel);
    }
    else
    {
        char buffer_auxiliar[BUFSIZ];
        snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "Operator necunoscut in clauza WHERE\n");
        send(clientSocket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
        printf("Operator necunoscut in clauza WHERE\n");
        pthread_rwlock_wrlock(&rwlock);
        return false;
    }
    return true;
}

bool comportamentDelete(SQLParser *parser, char *buffer, char *tableName, int clientSocket, Table *tabel)
{
    char *wherecol = (char *)malloc(20);
    char *whereval = (char *)malloc(20);
    char *whereop = (char *)malloc(20);
    parseDelete(parser, buffer, tableName, wherecol, whereval, whereop, clientSocket);

    tabel = loadTable(tableName);

    char **elemente = (char **)malloc(tabel->numarRanduri * sizeof(char *)); // elementele din coloana din clauza WHERE
    for (int j = 0; j < tabel->numarRanduri; j++)
        elemente[j] = (char *)malloc(10);
    int *colIndex = malloc(sizeof(int)); // indexul coloanei din clauza WHERE
    elemente = getElemByColumn(tabel, wherecol, colIndex);

    int *rowIndex = malloc(sizeof(int));
    *rowIndex = 0;
    BSTNode *root = buildBST(elemente, tabel->numarRanduri, *colIndex, rowIndex);

    BSTNode **searched = (BSTNode **)malloc(10 * sizeof(BSTNode *));
    int found = 0;

    int *rowmap = (int *)malloc(tabel->numarRanduri * sizeof(int));
    for (int i = 0; i < tabel->numarRanduri; i++)
        rowmap[i] = i;

    if (strcmp(whereop, "=") == 0)
    {
        searched = findNodesWithValue(root, whereval, &found);
        for (int i = 0; i < found; i++)
        {
            int row = searched[i]->row;
            for (int j = row + 1; j < tabel->numarRanduri; j++)
                rowmap[j]--;
            stergeRand(tabel, rowmap[row]);
        }
        scrieTabelInFisier(tabel->numeTabel, tabel);
    }
    else if (strcmp(whereop, "<=") == 0 || strcmp(whereop, ">=") == 0 || strcmp(whereop, "<") == 0 || strcmp(whereop, ">") == 0)
    {
        int n = 0;
        searched = getNodesByCondition(root, whereval, whereop, &n);
        for (int i = 0; i < n; i++)
        {
            int row = searched[i]->row;
            for (int j = row + 1; j < tabel->numarRanduri; j++)
                rowmap[j]--;
            stergeRand(tabel, rowmap[row]);
        }
        scrieTabelInFisier(tabel->numeTabel, tabel);
    }
    else if (strcmp(whereop, "!=") == 0)
    {
        int n = 0;
        searched = getNodesExcluding(root, whereval, &n);
        for (int i = 0; i < n; i++)
        {
            int row = searched[i]->row;
            for (int j = row + 1; j < tabel->numarRanduri; j++)
                rowmap[j]--;
            stergeRand(tabel, rowmap[row]);
        }
        scrieTabelInFisier(tabel->numeTabel, tabel);
    }
    else
    {
        char buffer_auxiliar[BUFSIZ];
        snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "Operator necunoscut in clauza WHERE\n");
        send(socket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
        printf("Operator necunoscut in clauza WHERE\n");
        pthread_rwlock_unlock(&rwlock);
        return false;
    }
    return true;
}

void handleClient(int clientSocket)
{
    char buffer[BUFFER_SIZE];
    int bytesRead;
    Table *tabel = NULL;
    SQLParser *parser = (SQLParser *)malloc(sizeof(SQLParser));
    if (parser == NULL)
    {
        printf("Eroare la alocarea memoriei pentru SQLParser.\n");
        return;
    }
    initSQLParser(parser);
    char titleTabel[30];
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);

        buffer[bytesRead] = '\0';
        int len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
        {
            buffer[len - 1] = '\0';
        }
        len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\r')
        {
            buffer[len - 1] = '\0';
        }
        printf("BUFFERUL INAINTE DE PRELUCRARE ESTE: %s\n", buffer);
        char tableName[40];
        int switching = parser->parse(parser, buffer);
        if (strcmp(buffer, "QUIT") == 0)
        {
            printf("Clientul a trimis comanda QUIT. Se închide conexiunea.\n");
            break;
        }

        switch (switching)
        {
        case 0:
        { // SELECT
            pthread_rwlock_rdlock(&rwlock);
            CacheEntry *entry = findInCache(cache, buffer);
            char *result=(char*)malloc(MAX_STRING_LENGTH);
            if (entry)
            {
                pthread_mutex_lock(&display_mutex);
                send(clientSocket, entry->result, strlen(entry->result), 0);
                printf("%s",entry->result);
                pthread_mutex_unlock(&display_mutex);
                printf("Rezultatul SELECT a fost preluat din cache.\n");
            }
            else
            {

                bool checking = comportamentSelect(clientSocket, buffer, tabel, parser, titleTabel, result);
                printf("REZULTAT TRECUT IN CACHE: %s", result);

                if (!checking)
                {
                    // a avut loc o eroare
                }
                addToCache(cache, buffer, result);
                pthread_rwlock_unlock(&rwlock);
                break;
            }
        }
        case 1:
        { // INSERT
            pthread_rwlock_wrlock(&rwlock);
            parseInsert(parser, buffer, clientSocket);
            pthread_rwlock_unlock(&rwlock);
            break;
        }
        case 2:
        { // UPDATE
            pthread_rwlock_wrlock(&rwlock);
            bool checking = comportamentUpdate(parser, buffer, tableName, tabel, clientSocket);
            if (!checking)
            {
            }
            pthread_rwlock_wrlock(&rwlock);
            break;
        }
        case 3:
        { // DELETE
            pthread_rwlock_wrlock(&rwlock);
            bool checking = comportamentDelete(parser, buffer, tableName, clientSocket, tableName);
            if (!checking)
            {
            }
            pthread_rwlock_unlock(&rwlock);
            break;
        }
        case 4:
        { // CREATE
            pthread_rwlock_wrlock(&rwlock);
            tabel = parseCreateTable(parser, buffer, clientSocket);
            pthread_rwlock_unlock(&rwlock);
            break;
        }
        default:
            char buffer_auxiliar[BUFSIZ];
            snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "Comandă necunoscută: %s\n", buffer);
            send(socket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
            printf("Comandă necunoscută: %s\n", buffer);
            break;
        }
    }
    free(parser);
    printf("Se inchide conexiunea!\n");
    close(clientSocket);
}

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;
    cache = createCache();

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("Eroare la crearea socket-ului");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Eroare la legarea socket-ului");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 10) < 0)
    {
        perror("Eroare la ascultarea socket-ului");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    ThreadPool *pool = createThreadPool(THREAD_COUNT);
    if (!pool)
    {
        perror("Eroare la crearea thread pool-ului");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Serverul este in asteptare...\n");

    while (1)
    {
        addr_size = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addr_size);
        if (clientSocket < 0)
        {
            perror("Eroare la acceptarea clientului");
            continue;
        }

        printf("Client conectat\n");

        ClientTask *task = (ClientTask *)malloc(sizeof(ClientTask));
        task->clientSocket = clientSocket;

        if (addTaskToPool(pool, handleClientWrapper, task) < 0)
        {
            printf("Thread pool este plin. Client respins.\n");
            close(clientSocket);
            free(task);
        }
    }

    destroyThreadPool(pool);
    pthread_rwlock_destroy(&rwlock);
    close(serverSocket);
    return 0;
}