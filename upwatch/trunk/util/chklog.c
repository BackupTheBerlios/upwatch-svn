#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <chklog_options.h>
#include "generic.h"
#include "logregex.h"

int debug = 0;

int main (int argc, char *argv[])
{
  int arg_ct;
  FILE *in;
  char buffer[2048];
  int line=0;

  arg_ct = optionProcess( &progOptions, argc, argv );
  argc -= arg_ct;
  argv += arg_ct;

  if (argc < 0) {
    fprintf(stderr, "Usage: chklog [-v 0-9] type:arg [type:arg] ..\n");
    exit (1);
  }

  logregex_refresh("/etc/upwatch.d/uw_sysstat.d");

  in = fopen(*argv, "r");
  if (!in) {
    perror(*argv);
    exit(1);
  }
  while (fgets(buffer, sizeof(buffer), in)) {
    int color = STAT_GREEN;

    line++;
    buffer[strlen(buffer)-1] = 0;
    if (HAVE_OPT(REVERSE)) {
      logregex_rmatchline("syslog", buffer);
      printf("%s\n", buffer);
    } else {
      if (logregex_matchline("syslog", buffer, &color)) {
        printf("line %u: %d: %s\n", line, color, buffer);
      }
    }
  }
  fclose(in);
  return 0;
}

