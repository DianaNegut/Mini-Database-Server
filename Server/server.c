
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

#define PORT 8125
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
    snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "\033[35mNumele tabelului este: %s\033[0m\n", nume_tabel);
    send(clientSocket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
    printf("\033[35mNumele tabelului este: %s\033[0m\n", nume_tabel);
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
        printf("\033[31mOperator necunoscut in clauza WHERE\n\033[0m");

        char coloredMessage[1024];
        snprintf(coloredMessage, sizeof(coloredMessage), "\033[31mOperator necunoscut în clauza WHERE\n\033[0m");
        send(clientSocket, coloredMessage, strlen(coloredMessage), 0);

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
    char *result_aux_ptr;
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
            strcpy(result_aux, "\033[31mNu s-a găsit un tabel în interogare.\n\033[0m");
            result_aux_ptr = strdup(result_aux);

            pthread_mutex_lock(&display_mutex);
            send(clientSocket, "\033[31mNu s-a găsit un tabel în interogare.\n\033[0m", strlen("\033[31mNu s-a găsit un tabel în interogare.\n\033[0m"), 0);
            printf("\033[31mNu s-a găsit un tabel în interogare.\n\033[0m");
            pthread_mutex_unlock(&display_mutex);

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
            snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "\033[32mDin coloana %s schimbam elementul %s de pe randul %d\n\033[0m", setcol, tabel->randuri[row].elemente[setColIndex], row + 1);
            send(clientSocket, buffer_auxiliar, strlen(buffer_auxiliar), 0);

            printf("\033[32mDin coloana %s schimbam elementul %s de pe randul %d\n\033[0m", setcol, tabel->randuri[row].elemente[setColIndex], row + 1);

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
            snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "\033[32mDin coloana %s schimbam elementul %s de pe randul %d\n\033[0m", setcol, tabel->randuri[row].elemente[setColIndex], row + 1);
            send(clientSocket, buffer_auxiliar, strlen(buffer_auxiliar), 0);
            printf("\033[32mDin coloana %s schimbam elementul %s de pe randul %d\n\033[0m", setcol, tabel->randuri[row].elemente[setColIndex], row + 1);
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
            snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "\033[32mDin coloana %s schimbam elementul %s de pe randul %d\n\033[0m",
                     setcol, tabel->randuri[row].elemente[setColIndex], row + 1);
            send(clientSocket, buffer_auxiliar, strlen(buffer_auxiliar), 0);

            printf("\033[32mDin coloana %s schimbam elementul %s de pe randul %d\n\033[0m",
                   setcol, tabel->randuri[row].elemente[setColIndex], row + 1);

            tabel->randuri[row].elemente[setColIndex] = setval;
        }
        scrieTabelInFisier(tableName, tabel);
    }
    else
    {
        char buffer_auxiliar[BUFSIZ];
        snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "\033[31mOperator necunoscut in clauza WHERE\n\033[0m");
        send(clientSocket, buffer_auxiliar, strlen(buffer_auxiliar), 0);

        printf("\033[31mOperator necunoscut in clauza WHERE\n\033[0m");

        pthread_rwlock_wrlock(&rwlock);
        return false;
    }
    free(wherecol);
    free(whereval);
    free(whereop);
    free(setcol);
    free(setval);
    freeBST(root);
    // for(int i = 0; i < found; i++)
    //     freeBST(searched[i]);
    // free(searched);
    //freeBST(*searched);
    // free(col);
    // free(root);
    // free(elemente);
    // free(colIndex);
    // free(rowIndex);
    // free(searched);
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
        snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "\033[31mOperator necunoscut in clauza WHERE\n\033[0m");
        send(socket, buffer_auxiliar, strlen(buffer_auxiliar), 0);

        printf("\033[31mOperator necunoscut in clauza WHERE\n\033[0m");

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
        printf("\033[31mEroare la alocarea memoriei pentru SQLParser.\n\033[0m");
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
        printf("\033[32mBUFFERUL INAINTE DE PRELUCRARE ESTE: %s\n\033[0m", buffer);

        char tableName[40];
        int switching = parser->parse(parser, buffer);
        if (strcmp(buffer, "QUIT") == 0)
        {
            printf("\033[35mClientul a trimis comanda QUIT. Se închide conexiunea.\n\033[0m");
            break;
        }

        switch (switching)
        {
        case 0:
        { // SELECT
            pthread_rwlock_rdlock(&rwlock);
            CacheEntry *entry = findInCache(cache, buffer);
            char *result = (char *)malloc(MAX_STRING_LENGTH);
            if (entry)
            {
                pthread_mutex_lock(&display_mutex);
                send(clientSocket, entry->result, strlen(entry->result), 0);
                printf("%s", entry->result);
                pthread_mutex_unlock(&display_mutex);
                printf("Rezultatul SELECT a fost preluat din cache.\n");
            }
            else
            {

                bool checking = comportamentSelect(clientSocket, buffer, tabel, parser, titleTabel, result);
                printf("\033[33mREZULTAT TRECUT IN CACHE: %s\033[0m", result);

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
            snprintf(buffer_auxiliar, sizeof(buffer_auxiliar), "\033[31mComandă necunoscută: %s\n\033[0m", buffer);
            send(socket, buffer_auxiliar, strlen(buffer_auxiliar), 0);

            printf("\033[31mComandă necunoscută: %s\n\033[0m", buffer);

            break;
        }
    }
    free(parser);
    printf("\033[31mSe inchide conexiunea!\n\033[0m");
    close(clientSocket);
}

void readMasterFile(const char *masterFilename, char files[][1024], int *fileCount)
{
    FILE *masterFile = fopen(masterFilename, "r");
    if (!masterFile)
    {
        perror("Eroare la deschiderea fișierului master");
        return;
    }

    char line[1024];
    *fileCount = 0;

    while (fgets(line, sizeof(line), masterFile))
    {
        line[strcspn(line, "\n")] = 0;
        strcpy(files[*fileCount], line);
        (*fileCount)++;
    }

    fclose(masterFile);
}

void copyFile(const char *source, const char *destination)
{
    FILE *src = fopen(source, "r");
    if (!src)
    {
        perror("Eroare la deschiderea fișierului sursă");
        return;
    }

    FILE *dst = fopen(destination, "w");
    if (!dst)
    {
        perror("Eroare la deschiderea fișierului destinație");
        fclose(src);
        return;
    }

    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), src)) > 0)
    {
        fwrite(buffer, 1, bytesRead, dst);
    }

    fclose(src);
    fclose(dst);
}
void *backupThread(void *arg)
{
    const char *masterFilename = "master";
    const char *backupDir = "backup";

    while (1)
    {
        struct stat st = {0};
        if (stat(backupDir, &st) == -1)
        {
            if (mkdir(backupDir, 0755) < 0)
            {
                perror("Eroare la crearea directorului de backup");
                sleep(300);
                continue;
            }
        }
        char files[100][1024];
        int fileCount = 0;
        readMasterFile(masterFilename, files, &fileCount);
        for (int i = 0; i < fileCount; i++)
        {
            char backupPath[1024];
            snprintf(backupPath, sizeof(backupPath), "%s/%s", backupDir, files[i]);
            copyFile(files[i], backupPath);
        }
        sleep(300);
    }

    return NULL;
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;
    cache = createCache();

    pthread_t backup_tid;
    if (pthread_create(&backup_tid, NULL, backupThread, NULL) != 0) {
        perror("Eroare la crearea thread-ului de backup");
        exit(EXIT_FAILURE);
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Eroare la crearea socket-ului");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        fprintf(stderr, "\033[31m");
        perror("Eroare la legarea socket-ului");
        fprintf(stderr, "\033[0m");

        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 10) < 0) {
        fprintf(stderr, "\033[31m");
        perror("Eroare la ascultarea socket-ului");
        fprintf(stderr, "\033[0m");

        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    ThreadPool *pool = createThreadPool(THREAD_COUNT);
    if (!pool) {
        fprintf(stderr, "\033[31m");
        perror("Eroare la crearea thread pool-ului");
        fprintf(stderr, "\033[0m");

        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("\033[34mServerul este in asteptare...\n\033[0m");

    while (1) {
        addr_size = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addr_size);
        if (clientSocket < 0) {
            perror("Eroare la acceptarea clientului");
            continue;
        }

        printf("Client conectat\n");

        ClientTask *task = (ClientTask *)malloc(sizeof(ClientTask));
        task->clientSocket = clientSocket;

        if (addTaskToPool(pool, handleClientWrapper, task) < 0) {
            printf("\033[31mThread pool este plin. Client respins.\n\033[0m");

            close(clientSocket);
            free(task);
        }
    }

    destroyThreadPool(pool);
    pthread_rwlock_destroy(&rwlock);
    close(serverSocket);

    pthread_cancel(backup_tid);
    pthread_join(backup_tid, NULL);

    return 0;
}
