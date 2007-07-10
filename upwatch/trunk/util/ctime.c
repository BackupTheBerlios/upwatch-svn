#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "ctime_options.h"

int main (int argc, char *argv[])
{
  int arg_ct;
  time_t arg;
  int c, idx=0;
  char number[40];

#ifdef ERRSKIP_OPTERR
  ERRSKIP_OPTERR;  // Don't barf on unknown commandline or configfile options. Only available in newer autogen versions
#endif

  arg_ct = optionProcess( &progOptions, argc, argv );
  argc -= arg_ct;
  argv += arg_ct;

  if (argc > 0) {
    arg = atoi(*argv);
    printf("%s", ctime(&arg));
    exit(0);
  }

  memset(number, 0, sizeof(number));
  while ((c = getc(stdin)) != EOF) {
    if (!isdigit(c)) { 
      if (idx) {
        fputs(number, stdout);
        memset(number, 0, sizeof(number));
        idx = 0;
      }
      putc(c, stdout); 
      continue; 
    }
    number[idx++] = c;
    if (atoi(number) > 1262300400) { // Jan 1st 2010
      fputs(number, stdout);
      memset(number, 0, sizeof(number));
      idx = 0;
      continue;
    }
    if (atoi(number) < 1009839600) {
      continue;
    }
    fputs(number, stdout);
    arg = atoi(number);
    sprintf(number, "%s", ctime(&arg));
    number[strlen(number)-1] = 0;
    putc('(', stdout);
    fputs(number, stdout);
    putc(')', stdout);
    memset(number, 0, sizeof(number));
    idx = 0;
  }
  return 0;
}

