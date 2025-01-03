#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8114
#define BUFFER_SIZE 1024

void trimiteCerereCreareTabel(int socket) {
    const char* numeTabel = "TEST_29.11.2024";


    char* coloane[] = {
        "id INT",
        "nume VARCHAR (20)"
    };

    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "CREATE TABLE %s ", numeTabel);
    for (int i = 0; i < sizeof(coloane) / sizeof(coloane[0]); i++) {
        strcat(buffer, " ");
        strcat(buffer, coloane[i]);
    }

    //snprintf(buffer, sizeof(buffer), "INSERT INTO angajati (id, nume, varsta, salariu) VALUES (\"A123\", 1, '30', 5000);");
    snprintf(buffer, sizeof(buffer), "INSERT INTO angajati (id, nume, varsta, salariu) VALUES (7, \"ANA m\", 30, 5000);");
    //snprintf(buffer, sizeof(buffer), "INSERT INTO Clienti (IDClient, Nume, Prenume) VALUES (12, \"Cinar\", \"Vasi\");");

    //snprintf(buffer, sizeof(buffer), "UPDATE Clienti SET IDClient = 0 WHERE Nume > \"Negut\"");
    //snprintf(buffer, sizeof(buffer), "UPDATE Clienti SET Prenume = \"Alexandra\" WHERE Nume <= \"Negut\"");

    //snprintf(buffer, sizeof(buffer), "SELECT Prenume, Nume FROM angajati WHERE IDClient < 3");
    //snprintf(buffer, sizeof(buffer), "SELECT * FROM angajati ");

    // Trimiterea cererii către server
    send(socket, buffer, strlen(buffer), 0);
    printf("Cerere de creare a tabelului trimisă: %s\n", buffer);

    // Așteptarea răspunsului de la server
    char raspuns[BUFFER_SIZE];
    int valread = read(socket, raspuns, sizeof(raspuns) - 1);
    raspuns[valread] = '\0'; // Asigurarea terminatorului de string
    printf("Răspuns de la server: %s\n", raspuns);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Crearea socketului
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Eroare la crearea socket-ului \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Conversia IPv4 și IPv6 de la text la binar
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\n Eroare la conversia adresei \n");
        return -1;
    }

    // Conectarea la server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Eroare la conectarea la server \n");
        return -1;
    }

    // Trimiterea cererii de creare a tabelului
    trimiteCerereCreareTabel(sock);

    // Închide socketul
    close(sock);
    return 0;
}