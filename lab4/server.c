#include "chat.h"
#include <errno.h>
#include <signal.h>

// zmienna globalna do flagi odebrania sygnału
volatile sig_atomic_t clear_array_flag = 0;
void handle_sigusr1(int sig){
  clear_array_flag = 1;
}

int main() {
  // obsługa sygnału SIGUSR1
  signal(SIGUSR1, handle_sigusr1);
  int clients[MAX_CLIENTS];
  int client_count = 0;

  // głowna kolejka serwera
  key_t server_key = get_server_key();
  int server_qid = msgget(server_key, IPC_CREAT | 0666);
  if (server_qid == -1) {
    perror("Błąd tworzenia kolejki serwera");
    exit(1);
  }

  printf("Serwer uruchomiony (PID: %d). Oczekiwanie na wiadomości...\n", getpid());
  struct chat_msg msg;

  while (1) {

    if(clear_array_flag == 1){
      printf("\n[SYGNAŁ] Czyszczenie tablicy klientów! (usunięto %d osób)\n", client_count);
      client_count = 0;
      clear_array_flag = 0;
      
    }

    if (msgrcv(server_qid, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) {
            // Sprawdzamy, czy msgrcv wyrzuciło błąd, bo przerwał mu nasz sygnał (EINTR)
            if (errno == EINTR) {
                continue; // Przerwano sygnałem - wróć na początek pętli (tam wyczyścimy tablicę)
            } else {
                perror("Poważny błąd odbierania wiadomości");
                break;
            }
        }

    if (msg.mtype == MSG_INIT) {
      // nowy klient chce sie połączyć
      if (client_count < MAX_CLIENTS) {
        // otworzenie kolejki klienta na pdst klucza
        int client_qid = msgget(msg.client_queue_key, 0);

        clients[client_count] = client_qid; // zapis kolejki
        int new_id = client_count + 1;      // nadanie ID
        client_count++;

        // odesłanie ID do klienta
        struct chat_msg response;
        response.mtype = MSG_SERVER_RESPONSE;
        response.client_id = new_id;
        msgsnd(client_qid, &response, sizeof(response) - sizeof(long), 0);
        printf("Podłączono nowego kllienta. Nadano ID: %d\n", new_id);

      } else {
        printf("Serwer pełen. Odrzucono połączenie.");
      }
    } else if (msg.mtype == MSG_CHAT) {
      // wiadomość od klienta
      printf("Wiadomość od %d: %s\n", msg.client_id, msg.mtext);

      for (int i = 0; i < client_count; i++) {
        // wysyłanie do wszystkich oprócz nadawcy
        if ((i + 1) != msg.client_id) {
          struct chat_msg broadcast_msg = msg;
          broadcast_msg.mtype = MSG_CHAT;

          msgsnd(clients[i], &broadcast_msg,
                 sizeof(broadcast_msg) - sizeof(long), 0);
        }
      }
    }
  }
  return 0;
}