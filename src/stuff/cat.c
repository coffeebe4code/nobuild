#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cat_file(const char *file_path) {
  FILE *f = fopen(file_path, "r");
  if (f == NULL) {
    fprintf(stderr, "ERROR: could not open file %s: %s\n", file_path,
            strerror(errno));
    exit(1);
  }

  static char cat_buffer[4098];

  while (!feof(f)) {
    size_t n = fread(cat_buffer, sizeof(cat_buffer[0]),
                     sizeof(cat_buffer) / sizeof(cat_buffer[0]), f);
    fwrite(cat_buffer, sizeof(cat_buffer[0]), n, stdout);
  }

  fclose(f);
}
