#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pcreposix.h>
#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char *argv[])
{
  char *line;

  line = readline("Enter inputline to match: ");
  while (1) {
    int err;
    regex_t preg;
    char *regexp = readline("Enter regexp: ");
    if (!regexp) break;
    if (*regexp) add_history(regexp);
    if (strcmp(regexp, "quit") == 0) {
      break;
    }

    err = regcomp(&preg, regexp, REG_EXTENDED|REG_NOSUB|REG_ICASE);
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

