#include <stdio.h>
#include <string.h>
#include <sys/msg.h>

void mover_der(char* entrada, int movimiento){
  char temporal[100];
  int mover = 5 * movimiento;
  strncpy(temporal, entrada, 100);
  for(int c = 0; c < 100; c++){
    temporal[c] = (c < mover)? ' ' : entrada[c-mover];
  }
  strncpy(entrada, temporal, 100);
}


void ver_rotonda(int *calles, int num_calles, int max_carros, char (*rotonda_s)[2][100]) {
  struct msqid_ds buff;
  char* carrito []= {".-'--`-._ ", "'-O---O--'"};
  char* nada= "     ";
  for (int c = 0; c < num_calles; c++) {
    msgctl(calles[c], IPC_STAT, &buff);
    printf("CALLE %d  \t%s", calles[c], *rotonda_s[0]);
    printf("\nCARROS: %ld\t%s\n", buff.msg_qnum, *rotonda_s[1]);
  }
}
