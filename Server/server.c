#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "SQLParser.h"
#include "gestionareTabele.h"
#include "BST.h"
#define PORT 8105
#define BUFFER_SIZE 1024

// Table* parseCreateTable(char* buffer) {
//     Table *t = (Table *)malloc(sizeof(Table));
//     memset(t, 0, sizeof(Table));

//     // Tokenize the SQL query and parse the table creation logic
//     char *token = strtok(buffer + 7, " ");  // Skip "CREATE" and "TABLE"
//     if (token != NULL) {
//         // Parse table name
//         strncpy(t->numeTabel, token, sizeof(t->numeTabel) - 1);
//         t->numeTabel[sizeof(t->numeTabel) - 1] = '\0';

//         // Initialize number of columns
//         t->numarColoane = 0;

//         // Parse columns and types
//         token = strtok(NULL, " ");
//         while (token != NULL) {
//             char *columnType = token;
//             token = strtok(NULL, " ");
//             if (token == NULL) break;

//             if (strcmp(columnType, "INT") == 0) {
//                 t->coloane[t->numarColoane].tipDate = INT;
//                 t->coloane[t->numarColoane].varchar_length = 0;
//             }
//             else if (strcmp(columnType, "VARCHAR") == 0) {
//                 t->coloane[t->numarColoane].tipDate = VARCHAR;
//                 t->coloane[t->numarColoane].varchar_length = 255;
//             }
//             else if (strcmp(columnType, "DATE") == 0) {
//                 t->coloane[t->numarColoane].tipDate = DATE;
//                 t->coloane[t->numarColoane].varchar_length = 0;
//             }

//             // Extract column name
//             token = strtok(NULL, " ");
//             strcpy(t->coloane[t->numarColoane].numeColoana, token);
//             t->numarColoane++;
//             token = strtok(NULL, " ");
//         }
//     }

//     return t;
// }

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
            char *wherecol = (char *)malloc(20);
            char *whereval = (char *)malloc(20);
            char *whereop = (char *)malloc(3 * sizeof(char));
            char **columns = parser->parseSelect(parser, buffer, titleTabel, wherecol, whereval, whereop); // coloanele pe care fac select
            tabel = loadTable(titleTabel);

            char **elemente = (char *)malloc(tabel->numarRanduri * sizeof(char)); // elementele din coloana din clauza WHERE
            for (int j = 0; j < tabel->numarRanduri; j++)
                elemente[j] = (char *)malloc(10);
            int *colIndex = malloc(sizeof(int)); // indexul coloanei din clauza WHERE
            elemente = getElemByColumn(tabel, wherecol, colIndex);

            int *rowIndex = malloc(sizeof(int));
            *rowIndex = 0;
            BSTNode *root = buildBST(elemente, tabel->numarRanduri, *colIndex, rowIndex);

            printf("Coloana: %d\n", *colIndex);
            printf("Cuvant: %s pe randul %d\n", root->word, root->row);
            printf("Cuvant: %s pe randul %d\n", root->right->word, root->right->row);
            printf("Cuvant: %s pe randul %d\n", root->right->left->word, root->right->left->row);
            printf("Cuvant: %s pe randul %d\n", root->right->right->word, root->right->right->row);
            printf("Cuvant: %s pe randul %d\n", root->right->right->right->word, root->right->right->right->row);

            BSTNode **searched = (BSTNode *)malloc(10 * sizeof(BSTNode));
            int found = 0;

            if (strcmp(whereop, "=") == 0)
            {
                searched = findNodesWithValue(root, whereval, &found);
                for (int i = 0; i < found; i++)
                {
                    int row = searched[i]->row;
                    printf("Din coloana %s am gasit elementul %s pe randul %d\n", columns[0], tabel->randuri[row].elemente[1], row);
                }
            }
            else if (strcmp(whereop, "<=") == 0)
            {
            }
            else if (strcmp(whereop, ">=") == 0)
            {
            }
            else if (strcmp(whereop, "!=") == 0)
            {
            }
            else
            {
                printf("Operator necunoscut in clauza WHERE\n");
                break;
            }

            break;
        }
        case 1:
        { // INSERT
            parseInsert(parser, buffer);
            break;
        }
        case 2:
        { // UPDATE
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
        exit(EXIT_FAILURE);
    }
    if (listen(serverSocket, 10) < 0)
    {
        perror("Eroare la ascultarea socket-ului");
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

        handleClient(clientSocket);
    }

    close(serverSocket);
    return 0;
}
