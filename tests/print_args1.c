#include <stdio.h>
int main(int argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    if (i < argc)
      i++;
    if (i == argc)
      break;
    else
      printf("%s\n", argv[i]);
  }
  return 0;
}