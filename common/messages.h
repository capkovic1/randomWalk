#ifndef MESSAGES_H
#define MESSAGES_H

typedef enum {
  MSG_CONNECT,
  MSG_START_SIM,
  MSG_STOP_SIM,
  MSG_RESULT
} MessageTyp;

typedef struct {
  MessageTyp type;
  int data;
} Message;

#endif
