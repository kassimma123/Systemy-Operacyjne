#ifndef CHAT_H
#define CHAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_TEXT 512
#define MAX_CLIENTS 10

// Definicje typów komunikatów
#define MSG_INIT 1
#define MSG_CHAT 2
#define MSG_SERVER_RESPONSE 3

// struktura komunikatu
struct chat_msg {
  long mtype;
  int client_id;
  key_t client_queue_key;
  char mtext[MAX_TEXT];
};

// gen klucza
static inline key_t get_server_key() { return ftok(getenv("HOME"), 'S'); }

#endif
