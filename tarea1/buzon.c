#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/msg.h>

#define MSG_SIZE 100

struct mensaje {
  long int type;        // Tipo de mensaje
  char mtext[MSG_SIZE]; // Contenido del mensaje
};

int enviar_msg(int buzon, char *msg, int tipo) {
  struct mensaje buff;
  strncpy(buff.mtext, msg, MSG_SIZE);
  buff.type = tipo;
  int error = 0;
  // Envia el mensaje
  if (msgsnd(buzon, &buff, MSG_SIZE, 0) == 0) {
    // printf("\t -> Se envió un mensaje correctamente.\n");
  } else {
    printf("\tHubo un error al ENVIAR un mensaje: %s\n [%d]", strerror(errno),
           tipo);
    error = 1;
  }
  return 0;
}

struct mensaje recibir_msg(int buzon, int tipo) {
  struct mensaje msg;
  // Recibe el mensaje
  if (msgrcv(buzon, &msg, MSG_SIZE, 0, 0)) {
    // printf("\t <- Se recibió un mensaje correctamente.\n");
  } else {
    printf("\tHubo un error al RECIBIR un mensaje: %s\n", strerror(errno));
  }
  return msg;
}
