#include "chat.h"

int main(){
    //tworzenie unkalnej kolejki klienta
    key_t client_key = ftok(getenv("HOME"), getpid());
    int client_qid = msgget(client_key, IPC_CREAT | 0666);
    int server_qid = msgget(get_server_key(), 0);

    //połączenie kolejki do serwera
    if(server_qid == -1){
        printf("błąd: nie znaleziono serwera! Upewnij sie ze jest uruchomiony.\n");
        return 1;
    }

    //wysłanie komunikatu INIT do serwera
    struct chat_msg msg;
    msg.mtype = MSG_INIT;
    msg.client_queue_key = client_key;
    msgsnd(server_qid, &msg, sizeof(msg) - sizeof(long), 0);

    //oczekiwanie na odpoweidź z ID od serwera
    msgrcv(client_qid, &msg, sizeof(msg) - sizeof(long), MSG_SERVER_RESPONSE, 0);
    int my_id = msg.client_id;
    printf("Połączono z serwerem. Twoje ID to: %d\n", my_id);

    //podział procesu fork na read i wrirte
    pid_t pid = fork();

    if (pid == 0){
        //proces dziecka - odebranie wiadomości z serwera
        while(1){
            msgrcv(client_qid, &msg, sizeof(msg) - sizeof(long), MSG_CHAT, 0);

            if (msg.client_id != my_id){
                printf("[Klient %d]: %s\n", msg.client_id, msg.mtext);
            }
        }
    }else if (pid > 0){
        //prces rodzica - wysyłanie wiad na serwer
        while(1){
            struct chat_msg out_msg;
            out_msg.mtype = MSG_CHAT;
            out_msg.client_id = my_id;

            fgets(out_msg.mtext, MAX_TEXT, stdin);

            msgsnd(server_qid, &out_msg, sizeof(out_msg) - sizeof(long), 0);
        }
    }
    return 0;
}