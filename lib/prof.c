#include <stdio.h>
#include <string.h>

void print_data(char *function_name, int function_size, int *count_array,
                int *index_array, int size) {
  char output_name[64];
  char ext[] = ".prof";
  memset(output_name, '\0', 64);
  memcpy(output_name, function_name, function_size);
  strcat(output_name, ext);

  FILE *file = fopen(output_name, "w");
  if (!file) {
    perror("Could not open file");
    return;
  }
  printf("Writing '%s'...\n", output_name);
  for (int i = 0; i < size; i++) {
    fprintf(file, "%d : %d\n", index_array[i], count_array[i]);
  }
  fclose(file);
}