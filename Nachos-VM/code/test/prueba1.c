#include "syscall.h"
void SimpleThread(int);
int main(int argc, char *argv[]) {
  // OpenFileId input = ConsoleInput;
  OpenFileId output = ConsoleOutput;
  char prompt[2], ch, buffer[60];
  int i;
  prompt[0] = '$';
  prompt[1] = '\n';
  Write(prompt, 2, output);
  // i = 0;
  // do {
  //   Read(&buffer[i], 1, input);
  // } while (buffer[i++] != '\n');
  // buffer[--i] = '\0';
  // if (i > 0) {
  //   Write(buffer, i, output);
  //   Write("\n", 1, output);
  // }
}
