#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "SQLParser.h"
#include "gestionareTabele.h"
#include "threadPool.h"
#include "BST.h"


#define PORT 8112
#define BUFFER_SIZE 1024
#define THREAD_COUNT 4
#define QUEUE_SIZE 10

typedef struct ClientTask {
    int clientSocket;
} ClientTask;


void handleClientWrapper(void *arg) {
    ClientTask *task = (ClientTask *)arg;
    handleClient(task->clientSocket);
    free(task);
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
    while ((bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1)) > 0)
    {
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
        char tableName[40];
        int switching = parser->parse(parser, buffer);

        switch (switching)
        {
        case 0:
        {
            char *copie_buffer = strdup(buffer);
            char *token = strtok(buffer, "*");
            if(strcmp(token, "SELECT ") == 0)//if (token != NULL)
            {
                // face SELECT * FROM nume tabel
                char nume_tabel[50];
                char *from_ptr;
                from_ptr = strstr(copie_buffer, "FROM");
                if (from_ptr != NULL)
                {
                    sscanf(from_ptr + 5, "%s", nume_tabel);
                    printf("Numele tabelului este: %s\n", nume_tabel);
                    tabel = loadTable(nume_tabel);
                    afiseazaTabel(tabel);
                }
                else
                {
                    printf("Nu s-a găsit un tabel în interogare.\n");
                }
            }
            else
            {
                char *wherecol = (char *)malloc(20);
                char *whereval = (char *)malloc(20);
                char *whereop = (char *)malloc(3 * sizeof(char));
                char **columns = parser->parseSelect(parser, buffer, titleTabel, wherecol, whereval, whereop);

                tabel = loadTable(titleTabel);

                int* colindexes = (int*) malloc(5 * sizeof(int));
                int nrselectedcols = 0;
                if(strcmp(columns[0], "*") != 0){
                    int i = 0;
                    while(columns[i] != NULL){
                        Column* col = (Column*) malloc(sizeof(Column));
                        col = getColumnByName(tabel, (const char*)columns[i]);
                        colindexes[i] = getColumnIndex(tabel, col);
                        nrselectedcols++;
                        i++;
                    }
                }
                else
                {
                    nrselectedcols = tabel->numarColoane;
                    for(int i = 0; i < nrselectedcols; i++){
                        colindexes[i] = i;
                        columns[i] = strdup(tabel->coloane[i].numeColoana);
                    }
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

                if(strcmp(whereop, "=") == 0){
                        searched = findNodesWithValue(root, whereval, &found);
                        for(int i = 0; i < found; i++){
                            int row = searched[i]->row;
                            printf("Din coloana %s am gasit elementul %s pe randul %d\n", columns[0], tabel->randuri[row].elemente[1], row+1);
                        }
                }else if(strcmp(whereop, "<=") == 0 || strcmp(whereop, ">=") == 0 || strcmp(whereop, "<") == 0 || strcmp(whereop, ">") == 0){
                        int n = 0;
                        searched = getNodesByCondition(root, whereval, whereop, &n);
                        for(int i = 0; i < n; i++){
                            int row = searched[i]->row;
                            printf("Din coloana %s am gasit elementul %s pe randul %d\n", columns[0], tabel->randuri[row].elemente[1], row+1);
                        }
                }else if(strcmp(whereop, "!=") == 0){
                        int n = 0;
                        searched = getNodesExcluding(root, whereval, &n);
                        for(int i = 0; i < n; i++){
                            int row = searched[i]->row;
                            printf("Din coloana %s am gasit elementul %s pe randul %d\n", columns[0], tabel->randuri[row].elemente[1], row+1);
                        }
                }
                    else{
                        printf("Operator necunoscut in clauza WHERE\n");
                        break;
                    }

                    break;
            }
        }
        case 1:
        { // INSERT
            parseInsert(parser, buffer);
            break;
        }
        case 2:
        { // UPDATE
            char* wherecol = (char*)malloc(20);
            char* whereval = (char*)malloc(20);
            char* whereop = (char*)malloc(20);
            char* setcol = (char*)malloc(20);
            char* setval = (char*)malloc(20);
            parseUpdate(parser, buffer, tableName, setcol, setval, wherecol, whereval, whereop);

            tabel = loadTable(tableName);

                char** elemente = (char**)malloc(tabel->numarRanduri * sizeof(char*)); //elementele din coloana din clauza WHERE
                    for(int j = 0; j < tabel->numarRanduri; j++)
                        elemente[j] = (char*)malloc(10);
                    int* colIndex = malloc(sizeof(int)); //indexul coloanei din clauza WHERE
                    elemente = getElemByColumn(tabel, wherecol, colIndex);

                    int* rowIndex = malloc(sizeof(int));
                    *rowIndex = 0;
                    BSTNode* root = buildBST(elemente, tabel->numarRanduri, *colIndex, rowIndex);

                    BSTNode** searched = (BSTNode**)malloc(10 * sizeof(BSTNode*));
                    int found = 0;

                    Column* col = malloc(sizeof(Column));
                    col = getColumnByName(tabel, setcol);
                    int setColIndex = getColumnIndex(tabel, col);

                    if(strcmp(whereop, "=") == 0){
                        searched = findNodesWithValue(root, whereval, &found);
                        printf("%d\n", found);
                        for(int i = 0; i < found; i++){
                            int row = searched[i]->row;
                            printf("Din coloana %s schimbam elementul %s de pe randul %d\n", setcol, tabel->randuri[row].elemente[setColIndex], row+1);
                            tabel->randuri[row].elemente[setColIndex] = setval;
                        }
                        scrieTabelInFisier(tableName, tabel);
                    }else if(strcmp(whereop, "<=") == 0 || strcmp(whereop, ">=") == 0 || strcmp(whereop, "<") == 0 || strcmp(whereop, ">") == 0){
                        int n = 0;
                        searched = getNodesByCondition(root, whereval, whereop, &n);
                        for(int i = 0; i < n; i++){
                            int row = searched[i]->row;
                            printf("Din coloana %s schimbam elementul %s de pe randul %d\n", setcol, tabel->randuri[row].elemente[setColIndex], row+1);
                            tabel->randuri[row].elemente[setColIndex] = setval;
                        }
                        scrieTabelInFisier(tableName, tabel);
                    }else if(strcmp(whereop, "!=") == 0){
                        int n = 0;
                        searched = getNodesExcluding(root, whereval, &n);
                        for(int i = 0; i < n; i++){
                            int row = searched[i]->row;
                            printf("Din coloana %s schimbam elementul %s de pe randul %d\n", setcol, tabel->randuri[row].elemente[setColIndex], row+1);
                            tabel->randuri[row].elemente[setColIndex] = setval;
                        }
                        scrieTabelInFisier(tableName, tabel);
                    }
                    else{
                        printf("Operator necunoscut in clauza WHERE\n");
                        break;
                    }
            break;
        }
        case 3:
        { // DELETE
            break;
        }
        case 4:
        { // CREATE
            tabel = parseCreateTable(parser, buffer);
            break;
        }
        default:
            printf("Comandă necunoscută: %s\n", buffer);
            break;
        }
    }
    close(clientSocket);
}

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Eroare la crearea socket-ului");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Eroare la legarea socket-ului");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 10) < 0) {
        perror("Eroare la ascultarea socket-ului");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    ThreadPool *pool = createThreadPool(THREAD_COUNT, QUEUE_SIZE);
    if (!pool) {
        perror("Eroare la crearea thread pool-ului");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Serverul este in asteptare...\n");

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
            printf("Thread pool este plin. Client respins.\n");
            close(clientSocket);
            free(task);
        }
    }

    destroyThreadPool(pool);
    close(serverSocket);
    return 0;
}
