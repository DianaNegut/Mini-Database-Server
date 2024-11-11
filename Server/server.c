#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "SQLParser.h"
#include "gestionareTabele.h"
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
            tabel = loadTable("angajati");
            // char **columns = parser->parseSelect(parser, buffer, titleTabel);
            // if (columns != NULL)
            // {
            //     for (int i = 0; i < 3; i++)
            //     {
            //         if (columns[i] != NULL)
            //         {
            //             printf("Coloana %d: %s\n", i + 1, columns[i]);
            //             free(columns[i]); 
            //         }
            //     }
            //     free(columns);
            // }
            // else
            // {
            //     printf("Eroare la procesarea coloanelor.\n");
            // }
            // break;
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
            tabel = parseCreateTable(parser,buffer);
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
