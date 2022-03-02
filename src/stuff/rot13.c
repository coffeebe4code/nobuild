#include <ctype.h>
#include <stdio.h>

char rot13(char x) {
  if ('a' <= x && x <= 'z')
    return ((x - 'a') + 13) % 26 + 'a';
  if ('A' <= x && x <= 'Z')
    return ((x - 'A') + 13) % 26 + 'A';
  return x;
}

#define BUFFER_SIZE (640 * 1000)

char buffer[BUFFER_SIZE];
