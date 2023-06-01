#include <stdio.h>
int main(int argc, char **argv) {
  int sum = 0;
  for (int i = 0; i < argc; i++) {
    printf("%s\n", argv[i]);
    if (argv[i][0]=='0')
      sum++;
  }
  return sum;
}