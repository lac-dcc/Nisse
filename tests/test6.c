#include <stdlib.h>
int main(int argc, char **argv) {
  int sum = 0;
  const int N = atoi(argv[1]);
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      sum++;
    }
  }
  return sum;
}