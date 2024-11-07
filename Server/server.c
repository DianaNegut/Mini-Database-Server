#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "gestionareTabele.h"

#define PORT 8082
#define BUFFER_SIZE 1024

Table *createTabel(char *buffer)
{
    Table *t = (Table *)malloc(sizeof(Table));
    memset(t, 0, sizeof(Table));
    char numeTabel[BUFFER_SIZE];
    char *token = strtok(buffer + 7, " ");
    if (token != NULL)
    {
        strncpy(numeTabel, token, BUFFER_SIZE - 1);
        numeTabel[BUFFER_SIZE - 1] = '\0';
        strncpy(t->numeTabel, numeTabel, sizeof(t->numeTabel) - 1);
        t->numeTabel[sizeof(t->numeTabel) - 1] = '\0';
        token = strtok(NULL, " ");
        t->numarColoane = 0;
        t->numarRanduri = 0;
        while (token != NULL)
        {
            char *tipColoana = token;
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                break;
            }
            if (strcmp(tipColoana, "INT") == 0)
            {
                t->coloane[t->numarColoane].tipDate = INT;
                t->coloane[t->numarColoane].varchar_length = 0;
            }
            else if (strcmp(tipColoana, "VARCHAR") == 0)
            {
                t->coloane[t->numarColoane].tipDate = VARCHAR;
                t->coloane[t->numarColoane].varchar_length = 255;
            }
            else if (strcmp(tipColoana, "DATE") == 0)
            {
                t->coloane[t->numarColoane].tipDate = DATE;
                t->coloane[t->numarColoane].varchar_length = 0;
            }
            char aux[BUFFER_SIZE];

            strcpy(aux, token);
            int len = strlen(aux);
            if (len > 0 && (aux[len - 1] == '\n' || aux[len - 1] == '\r'))
            {
                aux[len - 1] = '\0';
            }
            len = strlen(aux);
            if (len > 0 && (aux[len - 1] == '\n' || aux[len - 1] == '\r'))
            {
                aux[len - 1] = '\0';
            }

            aux[strlen(aux)] = 0;
            strcpy(t->coloane[t->numarColoane].numeColoana, aux);
            t->numarColoane++;
            token = strtok(NULL, " ");
        }
    }
    return t;
}

void handleClient(int clientSocket)
{
    char buffer[BUFFER_SIZE];
    int bytesRead;

    while ((bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1)) > 0)
    {
        int len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
        {
            buffer[len - 1] = '\0';
        }
        Table *tabel;
        if (strncmp(buffer, "CREATE", 6) == 0)
        {
            tabel = createTabel(buffer);
            if (tabel)
            {
                write(clientSocket, "Tabel creat cu succes!", strlen("Tabel creat cu succes!"));
            }
            else
            {
                write(clientSocket, "Eroare la crearea tabelului!", strlen("Eroare la crearea tabelului!"));
            }
        }
        else if (strncmp(buffer, "SELECT * FROM", 13) == 0)
        {
            char filename[BUFFER_SIZE];
            sscanf(buffer + 14, "%s", filename);
            write(clientSocket, "Se va afisa tabelul dorit!", strlen("Se va afisa tabelul dorit!"));
        }
        else if (strncmp(buffer, "SAVE", 4) == 0)
        {
            char filename[BUFFER_SIZE];
            sscanf(buffer + 5, "%s", filename);
            salveazaTabel(tabel, filename);
            write(clientSocket, "Tabel salvat cu succes!", strlen("Tabel salvat cu succes!"));
        }
        else
        {
            write(clientSocket, "Comandă necunoscută!", strlen("Comandă necunoscută!"));
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
