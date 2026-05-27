#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_BUFFER 2048

int main(int argc, char *argv[]) {
    // Sprawdzamy argumenty
    if (argc != 4) {
        fprintf(stderr, "Użycie: %s <adres_IPv4> <port> <LICZBA>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int liczba = atoi(argv[3]);

    int client_fd;
    struct sockaddr_in server_addr;

    // 1. Utworzenie gniazda
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("Błąd tworzenia gniazda (socket)");
        exit(EXIT_FAILURE);
    }

    // 2. Konfiguracja adresu serwera
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    // Konwersja tekstowego adresu IP
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Nieprawidłowy adres IP / Błąd inet_pton");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // 3. Połączenie z serwerem
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Błąd połączenia z serwerem (connect)");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // 4. Wysłanie tekstu do serwera
    char message[100];
    snprintf(message, sizeof(message), "ZADANIE %d", liczba);

    ssize_t bytes_sent = send(client_fd, message, strlen(message), 0);
    if (bytes_sent == -1) {
        perror("Błąd podczas wysyłania (send)");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // 5. Odebranie odpowiedzi i wyświetlenie na stdout
    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);

    ssize_t bytes_received = recv(client_fd, buffer, MAX_BUFFER - 1, 0);
    if (bytes_received > 0) {
        // Wypisujemy odzyskaną wiadomość bezpośrednio z serwera (bez formatowania i znaków dodatkowych, jak kazał schemat)
        printf("%s", buffer);
        
        // Jeżeli zależy nam na nowej linii (jeśli serwer by jej nie dodawał) to można ewentualnie tutaj dodać:
        if(buffer[bytes_received - 1] != '\n') {
            printf("\n");
        }
    } else if (bytes_received == 0) {
        printf("[Klient] Serwer zamknął połączenie, brak odpowiedzi.\n");
    } else {
        perror("Błąd przy odbieraniu (recv)");
    }

    // 6. Zakończenie pracy klienta
    close(client_fd);
    return 0;
}
