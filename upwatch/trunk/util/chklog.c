#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <chklog_options.h>
#include "generic.h"
#include "logregex.h"

char *progname;
int debug = 0;

int main (int argc, char *argv[])
{
  int arg_ct;
  FILE *in;
  char buffer[2048];
  int line=0;

  if ((progname = strrchr(argv[0], '/')) != NULL) {
    progname++;
  } else {
    progname = argv[0];
  }

#ifdef ERRSKIP_OPTERR
  ERRSKIP_OPTERR;  // Don't barf on unknown commandline or configfile options. Only available in newer autogen versions
#endif

  arg_ct = optionProcess( &progOptions, argc, argv );
  argc -= arg_ct;
  argv += arg_ct;

  if (argc < 0) {
    fprintf(stderr, "Usage: chklog [-v 0-9] type:arg [type:arg] ..\n");
    exit (1);
  }

  setvbuf(stdout, (char *)NULL, _IOLBF, 0); // make stdout linebuffered

  logregex_refresh_type("/etc/upwatch.d/uw_sysstat.d", (char *) OPT_ARG(TYPE));

  if (strcmp(*argv, "-") == 0) {
    in = stdin;
  } else {
    in = fopen(*argv, "r");
  }
  if (!in) {
    perror(*argv);
    exit(1);
  }
  while (fgets(buffer, sizeof(buffer), in)) {
    int color = STAT_GREEN;

    line++;
    
    if (HAVE_OPT(STATS)) {
      if ((line <= 100 && line % 10 == 0) ||
         (line > 100 && line % 100 == 0)) {
        fprintf(stderr, "%7d\b\b\b\b\b\b\b", line);
      }
    }
    buffer[strlen(buffer)-1] = 0;
    if (HAVE_OPT(MATCH)) {
      if (logregex_matchline((char *) &OPT_ARG(TYPE), buffer, &color)) {
        if (HAVE_OPT(LINE_INFO)) {
          printf("line %u: %d: ", line, color);
        }
        printf("%s\n", buffer);
      }
    }
    if (HAVE_OPT(REVERSE)) {
      logregex_rmatchline((char *) &OPT_ARG(TYPE), buffer);
      printf("green %s\n", buffer);
    }
  }
  fclose(in);
  if (HAVE_OPT(STATS)) {
    logregex_print_stats((char *) &OPT_ARG(TYPE));
  }
  return 0;
}

