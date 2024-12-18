#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8126
#define BUFFER_SIZE 1024
#define NUM_CLIENTS 15 // Mai mare decât TASK_QUEUE_SIZE pentru a forța respingeri

void *clientThread(void *arg) {
    int client_id = *((int *)arg);
    int sock;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE] = {0};
    char command[100];

    // Creează socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Eroare la crearea socket-ului");
        return NULL;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Conectează-te la server
    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Eroare la conectarea la server");
        close(sock);
        return NULL;
    }

    // Trimite o comandă specifică
    snprintf(command, sizeof(command), "SELECT * FROM angajati WHERE id=%d\n", client_id);
    send(sock, command, strlen(command), 0);
    printf("[Client %d] Comanda trimisă: %s", client_id, command);

    // Primește răspunsul
    int bytesRead = read(sock, buffer, BUFFER_SIZE - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        printf("[Client %d] Răspuns primit: %s\n", client_id, buffer);
    }

    // Închide conexiunea
    close(sock);
    return NULL;
}

int main() {
    pthread_t threads[NUM_CLIENTS];
    int client_ids[NUM_CLIENTS];

    for (int i = 0; i < NUM_CLIENTS; i++) {
        client_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, clientThread, &client_ids[i]);
    }

    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
