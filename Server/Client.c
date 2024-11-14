#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8116
#define BUFFER_SIZE 1024

void trimiteCerereCreareTabel(int socket) {
    // Numele tabelului
    const char* numeTabel = "utilizatori";

    // Detalii despre coloane
    char* coloane[] = {
        "id INT",
        "nume VARCHAR(50)",
        "data_nasterii DATE"
    };

    // Construirea mesajului de cerere
    char buffer[BUFFER_SIZE];
    //snprintf(buffer, sizeof(buffer), "CREARE_TABEL %s %d", numeTabel, sizeof(coloane) / sizeof(coloane[0]));
    // for (int i = 0; i < sizeof(coloane) / sizeof(coloane[0]); i++) {
    //     strcat(buffer, " ");
    //     strcat(buffer, coloane[i]);
    // }

    snprintf(buffer, sizeof(buffer), "INSERT INTO angajati (id, nume, varsta, salariu) VALUES (\"A123\", 1, '30', 5000);");

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