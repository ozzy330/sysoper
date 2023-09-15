#include <sys/wait.h>
#include <unistd.h>

#include "buzon.c"
#include "random.c"
#include "carritos.c"

struct transito {
  int cantidad_carros;
  int id_calle;
};


int en_rotonda(int *calles, int num_calles, int max_carros) {
  struct msqid_ds buff;
  int carros = 0;
  for (int c = 0; c < num_calles; c++) {
    msgctl(calles[c], IPC_STAT, &buff);
    carros += buff.msg_qnum;
  }
  return carros;
}

void calle_congestionada(struct transito *t, int *calles, int num_calles,
                         int max_carros) {
  struct msqid_ds buff;
  int current_max = 0;
  for (int c = 0; c < num_calles; c++) {
    msgctl(calles[c], IPC_STAT, &buff);
    if (current_max < buff.msg_qnum) {
      t->id_calle = calles[c];
      t->cantidad_carros = buff.msg_qnum;
      current_max = buff.msg_qnum;
    }
  }
}

void liberar_calle(struct transito *t) {
  for (int c = 0; c < t->cantidad_carros; c++) {
    recibir_msg(t->id_calle, 1);
    // printf("\t>>> Liberando calle %d\n", t->id_calle);
  }
}

int llegar(int *calles, int num_calles, int max_carros, char (*rotonda_s)[2][100]) {
  printf("\033[H\033[J");
  int num_carros = en_rotonda(calles, num_calles, max_carros);
  if (num_carros > max_carros) {
    // printf("\t--- Ya hay muchos carros... (%d)\n", num_carros);
    struct transito t;
    calle_congestionada(&t, calles, num_calles, max_carros);
    // printf("\t--- La calle más congestionada es %d\n", t.id_calle);
    liberar_calle(&t);
  }
  // printf("*** La cantidad de carros en la rotonda es: %d\n", num_carros);
  int calle = num_random(num_calles);
  ver_rotonda(calles, num_calles, max_carros, rotonda_s);
  // printf("\n\nENVIAR MSG A %d\n\n", calle);
  enviar_msg(calles[calle], "", 1);
  return 0;
}

int rotonda(int num_calles, int max_carros, int simulacion) {
  // Crea las calles
  int calles[num_calles];
  for (int c = 0; c < num_calles; ++c) {
    int buzon_id = msgget(c, 0666 | IPC_CREAT);
    if (buzon_id >= 0) {
      calles[c] = buzon_id;
      printf("Se creó la calle #%d\n", calles[c]);
    } else {
      printf("Hubo un error al CREAR una calle [%d], erro: %s\n", c,
             strerror(errno));
      return EXIT_FAILURE;
    }
  }

  char (*rotonda_s)[2][100];

  for (int s = 0; s < simulacion; ++s) {
    int id = fork();
    if (id == 0) {
      sleep(1);
      llegar(calles, num_calles, max_carros, rotonda_s);
      exit(EXIT_SUCCESS); // Termina el proceso hijo
    } else {
      sleep(1);
      wait(NULL); // Espera hijos
    }
    if (s == simulacion - 1) {
      printf("\nPresione 'q' (terminar) o cualquier tecla (continuar) y luego "
             "'Intro': ");
      int tecla = getchar();
      printf("\n");
      s = (tecla == 'q' || tecla == 'Q') ? s : -1;
    }
  }

  // Elimina las calles
  for (int c = 0; c < num_calles; ++c) {
    msgctl(calles[c], IPC_RMID, NULL);
    printf("Se eliminó la calle #%d\n", calles[c]);
  }

  return EXIT_SUCCESS;
}
