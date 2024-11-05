#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "gestionareTabele.h" 

#define PORT 8088
#define BUFFER_SIZE 1024

void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE];
    int bytesRead;

    while ((bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0'; 
        Table* tabel;
        if (strncmp(buffer, "CREATE", 6) == 0) {
            char numeTabel[BUFFER_SIZE];
            Column coloane[1024]; 
            int numarColoane = 0;
            char* token = strtok(buffer + 7, " "); 
            if (token != NULL) {
                strncpy(numeTabel, token, BUFFER_SIZE); 
                token = strtok(NULL, " "); 
                while (token != NULL && numarColoane < 1024) {
                    char* tipColoana = token; 
                    token = strtok(NULL, " ");
                    if (token == NULL) {
                        break; 
                    }
                    if (strcmp(tipColoana, "INT") == 0) {
                        coloane[numarColoane].tipDate = INT;
                        coloane[numarColoane].varchar_length = 0; 
                    } else if (strcmp(tipColoana, "VARCHAR") == 0) {
                        coloane[numarColoane].tipDate = VARCHAR;
                        coloane[numarColoane].varchar_length = 255;
                    } else if (strcmp(tipColoana, "DATE") == 0) {
                        coloane[numarColoane].tipDate = DATE;
                        coloane[numarColoane].varchar_length = 0; 
                    }
                    strncpy(coloane[numarColoane].numeColoana, token, 100); 
                    coloane[numarColoane].numeColoana[strlen(coloane[numarColoane].numeColoana)]=0;
                    numarColoane++;
                    token = strtok(NULL, " "); 
                }
                tabel= creazaTabel(numeTabel, coloane, numarColoane);
                if (tabel) {
                    write(clientSocket, "Tabel creat cu succes!", strlen("Tabel creat cu succes!"));
                } else {
                    write(clientSocket, "Eroare la crearea tabelului!", strlen("Eroare la crearea tabelului!"));
                }
            } else {
                write(clientSocket, "Comandă incompletă!", strlen("Comandă incompletă!"));
            }
        } else if (strncmp(buffer, "SAVE", 4) == 0) {
            char filename[BUFFER_SIZE];
            sscanf(buffer + 5, "%s", filename); 
            salveazaTabel(tabel,filename );
            write(clientSocket, "Tabel salvat cu succes!", strlen("Tabel salvat cu succes!"));
        } else {
            write(clientSocket, "Comandă necunoscută!", strlen("Comandă necunoscută!"));
        }
    }

    close(clientSocket);
}

int main() {
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
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Eroare la legarea socket-ului");
        exit(EXIT_FAILURE);
    }
    if (listen(serverSocket, 10) < 0) {
        perror("Eroare la ascultarea socket-ului");
        exit(EXIT_FAILURE);
    }

    printf("Serverul este in asteptare...\n");

    while (1) {
        addr_size = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addr_size);
        if (clientSocket < 0) {
            perror("Eroare la acceptarea clientului");
            continue;
        }
        printf("Client conectat\n");

        handleClient(clientSocket);
    }

    close(serverSocket);
    return 0;
}
