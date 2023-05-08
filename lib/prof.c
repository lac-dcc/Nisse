#include <stdio.h>
#include <string.h>

void print_data(char *function_name, int *count_array, int *index_array,
                int size) {
  char buf[64];
  for (int i = 0; i < 64; i++) {
    buf[i] = '\0';
  }
  memcpy(buf, function_name, 63);
  printf("Function %s:\n", function_name);
  for (int i = 0; i < size; i++) {
    printf("\t%d : %d\n", index_array[i], count_array[i]);
  }
}