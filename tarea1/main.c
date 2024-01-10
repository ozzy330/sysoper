/*
 * N: calles de doble sentido (colas)
 * Carros: son procesos
 * t: cantidad de carros en todas las colas
 * K: constante de carros
 *
 * Si N[x] < K, esperar proceso
 * Si N[x] > K, finalizar todos los procesos hasta x actual
 *
 * Utilizando procesos y buzones
 */

#include "./rotonda.c"

#define SIMULACION 50

int main(int argc, char *argv[]) {

  int calles = 3;              // Cantidad de calles
  int max_carros = 13;          // Maxima cantidad de carros en rotonda
  int simulacion = SIMULACION; // Cuantos carros simular hasta preguntar si
                               // terminar la simulacion
  int opt;
  opterr = 0;
  while ((opt = getopt(argc, argv, "c:k:s:")) != -1) {
    switch (opt) {
    case 'c':
      calles = atoi(optarg);
      break;
    case 'k':
      max_carros = atoi(optarg);
      break;
    case 's':
      max_carros = atoi(optarg);
      break;
    case '?':
      printf("\nHubo un error.\n\nSolo se aceptan dos argumentos: \n\t-c "
             "<cantidad de calles> \n\t-k "
             "<cantidad maxima de carros en la rotonda>\n");
      return EXIT_FAILURE;
      break;
    }
  }
  printf("\033[H\033[J");
  printf("La cantidad de calles será %d. \n", calles);
  printf("La maxima cantidad de carros será %d. \n\n", max_carros);
  printf("Cada %d carros, se pausará la simulacion. Si desea continuar "
         "presione 'Intro'\nSi desea terminar la simulación presion 'q' y "
         "luego 'Intro'\n\n",
         max_carros);
  rotonda(calles, max_carros, simulacion);

  return EXIT_SUCCESS;
}
