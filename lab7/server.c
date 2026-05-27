#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_BUFFER 2048

// Zmienna globalna zgodna z opisem zadania
int LICZNIK_ZAPYTAN = 0;

void handle_client(int client_fd) {
    char buffer[MAX_BUFFER];
    char response[MAX_BUFFER];
    char response_body[MAX_BUFFER];

    // Zerujemy bufor
    memset(buffer, 0, MAX_BUFFER);

    // Odczyt zapytania
    ssize_t bytes_read = recv(client_fd, buffer, MAX_BUFFER - 1, 0);
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }

    // Szukamy spacji po początkowym komendzie, jednak wg. treści
    // wystarczy sprawdzić czy ZACZYNA SIĘ od "GET", "POST", "ZADANIE"
    if (strncmp(buffer, "GET", 3) == 0 || strncmp(buffer, "POST", 4) == 0) {
        LICZNIK_ZAPYTAN += 1;
        
        // Tworzymy tekst odpowiedzi (body)
        snprintf(response_body, MAX_BUFFER, "Liczba pobrań strony: %d\n", LICZNIK_ZAPYTAN);
        
        // Długość tekstu odpowiedzi
        int body_length = strlen(response_body);

        // Tworzymy nagłówek HTTP z wymaganym tekstem
        snprintf(response, MAX_BUFFER, 
            "HTTP/1.1 200 OK\r\n"
            "Server: Zajeciowy serwer SO\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            "Connection: close\r\n"
            "Cache-Control: no-store\r\n"
            "Content-Length: %d\r\n\r\n"
            "%s", 
            body_length, response_body);
        
        // Wysłanie odpowiedzi
        send(client_fd, response, strlen(response), 0);

    } else if (strncmp(buffer, "ZADANIE ", 8) == 0) {
        // Parsujemy liczbe
        int liczba = 0;
        if (sscanf(buffer, "ZADANIE %d", &liczba) == 1) {
            LICZNIK_ZAPYTAN += liczba;
        }

        // Odpowiedź jak wyżej, ale bez nagłówka HTTP
        snprintf(response_body, MAX_BUFFER, "Liczba pobrań strony: %d\n", LICZNIK_ZAPYTAN);
        
        // Wysłanie odpowiedzi bezpośrednio bez HTTP
        send(client_fd, response_body, strlen(response_body), 0);
    } else {
        // Ignorujemy zapytania których nie rozumiemy, np favicon.
        // Odpowiemy minimum zeby nie wieszac przegladarki jesli zapyta o cos innego
        snprintf(response, MAX_BUFFER, 
            "HTTP/1.1 400 Bad Request\r\n"
            "Connection: close\r\n\r\n"
            "Bad Request");
        send(client_fd, response, strlen(response), 0);
    }

    // Zamknięcie połączenia
    close(client_fd);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // 1. Tworzenie gniazda TCP
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Błąd tworzenia gniazda (socket)");
        exit(EXIT_FAILURE);
    }

    // Opcja pozwalająca na ponowne bindowanie portu od razu po restarcie programu
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Błąd ustawiania SO_REUSEADDR");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 2. Konfiguracja adresu serwera
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Nasłuch na wszystkich interfejsach
    server_addr.sin_port = htons(9000);       // Port 9000

    // 3. Powiązanie gniazda z portem (bind)
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Błąd powiązania (bind)");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 4. Tryb nasłuchiwania (listen)
    if (listen(server_fd, 10) == -1) {
        perror("Błąd nasłuchiwania (listen)");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[Serwer] Uruchomiono serwer na porcie 9000...\n");

    // 5. Nieskończona pętla
    while (1) {
        // Akceptacja nowego klienta
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("Błąd przyjmowania połączenia (accept)");
            continue;
        }

        // Możemy obsługiwać synchronicznie, blokując kolejnych dopóki nie obsłużymy aktualnego
        handle_client(client_fd);
    }

    // Choć przy nieskończonej pętli nigdy tu nie dojdzie
    close(server_fd);
    return 0;
}
