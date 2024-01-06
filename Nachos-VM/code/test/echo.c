#include "syscall.h"
int main() {
  OpenFileId input = ConsoleInput;
  OpenFileId output = ConsoleOutput;
  char ch, buffer[60];
  Write("> ", 2, output);
  int i = 0;
  do {
    Read(&buffer[i], 1, input);
  } while (buffer[i++] != '\n');
  buffer[--i] = '\0';
  if (i > 0) {
    Write(buffer, i, output);
    Write("\n", 1, output);
  }
}
