#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <logregex.h>
#include <readline/readline.h>
#include <readline/history.h>

int debug;

int main(int argc, char *argv[])
{
  char *line;

  line = readline("Enter inputline to match: ");
  while (1) {
    int err;
    char buf[8192];
    regex_t preg;
    char *regexp = readline("Enter regexp: ");
    if (!regexp) break;
    if (*regexp) add_history(regexp);
    if (strcmp(regexp, "quit") == 0) {
      break;
    }

    logregex_refresh("/etc/upwatch.d/uw_sysstat.d");
    logregex_expand_macros("syslog", regexp, buf);

    err = regcomp(&preg, buf, REG_EXTENDED|REG_NOSUB|REG_ICASE);
    if (err) {
      char buffer[256];

      regerror(err, &preg, buffer, sizeof(buffer));
      printf("%s\n", buffer);
      continue;
    }
    if (regexec(&preg,  line, 0, 0, 0) == 0) {
      printf("match!\n");
    } else {
      printf("no match!\n");
    }
    free(regexp);
  }
  printf("\n");
}

