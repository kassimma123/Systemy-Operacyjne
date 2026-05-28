#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_BUFFER 2048

int LICZNIK_ZAPYTAN = 0;

void handle_client(int client_fd) {
  char buffer[MAX_BUFFER];
  char response[MAX_BUFFER];
  char response_body[MAX_BUFFER];

  // zerujemy bufor
  memset(buffer, 0, MAX_BUFFER);

  // odczyt zapytania za pomocą read
  ssize_t bytes_read = read(client_fd, buffer, MAX_BUFFER - 1);
  if (bytes_read <= 0) {
    close(client_fd);
    return;
  }

  if (strncmp(buffer, "GET", 3) == 0 || strncmp(buffer, "POST", 4) == 0) {
    LICZNIK_ZAPYTAN += 1;

    // wpisujemy wartość zmiennej LICZNIK_ZAPYTAN
    snprintf(response_body, MAX_BUFFER, "Liczba pobrań strony: %d",
             LICZNIK_ZAPYTAN);

    // liczba bajtów z których składa się tekst odpowiedzi
    int body_length = strlen(response_body);

    // tekst odpowiedzi
    snprintf(response, MAX_BUFFER,
             "HTTP/1.1 200 OK\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n\r\n"
             "%s",
             body_length, response_body);
    printf("!%s!", response);
    // Wysłanie odpowiedzi całości (nagłówek + ciało) za pomocą write
    write(client_fd, response, strlen(response));

  } else if (strncmp(buffer, "ZADANIE", 7) == 0) {
    int liczba = 0;
    if (sscanf(buffer, "ZADANIE %d", &liczba) == 1 ||
        sscanf(buffer, "ZADANIE%d", &liczba) == 1) {
      LICZNIK_ZAPYTAN += liczba;
      printf("[Serwer] Otrzymano ZADANIE z liczbą: %d. Aktualny licznik: %d\n", liczba, LICZNIK_ZAPYTAN);
    }

    // tekst odpowiedzi jak powyżej
    snprintf(response_body, MAX_BUFFER, "Liczba pobrań strony: %d",
             LICZNIK_ZAPYTAN);

    // odesłać tekst odpowiedzi jak powyżej ale z pominięciem nagłówka HTTP
    write(client_fd, response_body, strlen(response_body));

  } else {
    // ignorujemy zapytania których nie rozumiemy
    snprintf(response, MAX_BUFFER,
             "HTTP/1.1 400 Bad Request\r\n"
             "Connection: close\r\n\r\n"
             "Bad Request");
    write(client_fd, response, strlen(response));
  }

  // zamknięcie połączenia
  close(client_fd);
}

int main() {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  // tworzenie gniazda TCP
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("Błąd tworzenia gniazda (socket)");
    exit(EXIT_FAILURE);
  }

  // opcja pozwalająca na ponowne bindowanie portu od razu po restarcie programu
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("Błąd ustawiania SO_REUSEADDR");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // konfiguracja adresu serwera
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr =
      INADDR_ANY;                     // nasłuch na wszystkich interfejsach
  server_addr.sin_port = htons(9000); // port 9000

  // powiązanie gniazda z portem (bind)
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("Błąd powiązania (bind)");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // tryb nasłuchiwania
  if (listen(server_fd, 10) == -1) {
    perror("Błąd nasłuchiwania (listen)");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("[Serwer] Uruchomiono serwer na porcie 9000...\n");

  // nieskończona pętla
  while (1) {
    // akceptacja nowego klienta
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
      perror("Błąd przyjmowania połączenia (accept)");
      continue;
    }

    // obsługa klienta synchronicznie, blokując kolejnych dopóki nie obsłużymy aktualnego
    handle_client(client_fd);
  }

  // choć przy nieskończonej pętli nigdy tu nie dojdzie
  close(server_fd);
  return 0;
}
