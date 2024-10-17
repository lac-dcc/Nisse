#include <stdio.h>
#include <string.h>
#include <unistd.h>

void print_data(char *function_name, int function_size, int *count_array,
                int *index_array, int size) {
  char output_name[64];
  char ext[] = ".prof";
  char line[] = "-------------------------------";
  memset(output_name, '\0', 64);
  memcpy(output_name, function_name, function_size);
  strcat(output_name, ext);

  if (access(output_name, F_OK) != 0) {
    printf("Writing '%s'...\n", output_name);
  }
  FILE *file = fopen(output_name, "ab");
  if (!file) {
    perror("Could not open file");
    return;
  }
  fwrite(&size, sizeof(int), 1, file);
  // fprintf(file, "%s%s%s\n", line, function_name, line);
  for (int i = 0; i < size; i++) {
    fwrite(&index_array[i], sizeof(int), 1, file);
    fwrite(&count_array[i], sizeof(int), 1, file);
    // int numbers[2] = {index_array[i], count_array[i]};
    // fwrite(numbers, sizeof(int), sizeof(numbers)/sizeof(int), file);
    // fprintf(file, "%d %d\n", index_array[i], count_array[i]);
  }
  fclose(file);
}