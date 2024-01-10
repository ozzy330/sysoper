#include <stdlib.h>
#include <time.h>
int num_random(int max_numero){
  srandom(time(NULL));
  return random() % max_numero;
}
