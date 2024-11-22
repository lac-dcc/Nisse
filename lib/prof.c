#include <stdio.h>
#include <string.h>
#include <unistd.h>

void nisse_pass_print_data(int *count_array, int *index_array, int size) {
  if (access("main.prof", F_OK) != 0) {
    printf("Writing '%s'...\n", "main.prof");
  }
  FILE *file = fopen("main.prof", "a");
  if (!file) {
    perror("Could not open file");
    return;
  }
  // fwrite(&size, sizeof(int), 1, file);
  // fwrite(index_array, sizeof(long long), size, file);
  // fwrite(count_array, sizeof(int), size, file);
  // fprintf(file, "%s%s%s\n", line, function_name, line);
  for (int i = 0; i < size; i++) {
    // fwrite(&index_array[i], sizeof(int), 1, file);
    // fwrite(&count_array[i], sizeof(int), 1, file);
    // int numbers[2] = {index_array[i], count_array[i]};
    // fwrite(numbers, sizeof(int), sizeof(numbers)/sizeof(int), file);
    fprintf(file, "%d %d\n", index_array[i], count_array[i]);
  } 
  fclose(file);
}