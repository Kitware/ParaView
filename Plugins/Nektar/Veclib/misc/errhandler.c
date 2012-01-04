#include <stdio.h>

void error_handler(error_text)
char error_text[];
{
  void exit();

  fprintf(stderr,"\nutility library: ");
  fprintf(stderr,"%s\n", error_text);
  fprintf(stderr,"...now exiting to system...\n\n");
  exit(1);
}
