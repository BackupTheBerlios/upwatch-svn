#pragma once

#define CONFIG_FILE "c:\\Program Files\\upwatch\\etc\\uw_sysstat.conf"
#define SPEC_FILE "c:\\Program Files\\upwatch\\etc\\uw_sysstat.spec"
#define STATFILE "c:/Program Files/upwatch/log/uw_sysstat.stat"
#define RECORD_FILE "c:\\Program Files\\upwatch\\log\\uw_sysstat.record"
#define SPOOL_DIRECTORY "c:\\Program Files\\upwatch\\spool"
#define LOG_DIRECTORY "c:\\Program Files\\upwatch\\log"

#define STAT_GREEN 200
#define STAT_YELLOW 300
#define STAT_PURPLE 400
#define STAT_RED 500
#define LOG_INFO 0
#define LOG_WARNING 1
#define LOG_ERR 2
#define PATH_MAX 260

#include <sys/stat.h>
#include ".\log.h"
#include ".\logregex.h"
#include ".\glib.h"

#pragma comment(lib, "glib.lib")
#pragma comment(lib, "libpcre.lib")

static struct _errlogspec {
  char *style;
  char *path;
  long long offset;
} *errlogspec = NULL;

static int check_log(GString *string, int idx, int *color)
{
  FILE *in;
  struct stat st;
  char buffer[8192];
  int firstmatch = TRUE;
  int logcolor = STAT_GREEN;

  in = fopen(errlogspec[idx].path, "r");
  if (!in) {
    errlogspec[idx].offset = 0;
    return STAT_GREEN;
  }
  if (fstat(fileno(in), &st)) {
    char buf2[PATH_MAX+4];

    sprintf(buf2, "%s: %m\n", errlogspec[idx].path);
    g_string_append(string, buf2);
    LOG(LOG_WARNING, buf2);
    fclose(in);
    errlogspec[idx].offset = 0;
    return STAT_YELLOW;
  }
  if (st.st_size < errlogspec[idx].offset) {
    errlogspec[idx].offset = 0;
  }
  fseek(in, (long)errlogspec[idx].offset, SEEK_SET);
  while (fgets(buffer, sizeof(buffer), in)) {
    int color;

    if (logregex_matchline(errlogspec[idx].style, buffer, &color)) {
      if (firstmatch) {
        char buf2[PATH_MAX+4];

        sprintf(buf2, "%s:\n", errlogspec[idx].path);
        g_string_append(string, buf2);
        firstmatch = FALSE;
      }
      g_string_append(string, buffer);
    }
    if (color > logcolor) logcolor = color;
  }
  errlogspec[idx].offset = st.st_size;
  fclose(in);
  return logcolor;
}

//
// check the system log for funny things. Set appropriate color
static GString *check_logs(int *color)
{
  GString* string;
  FILE *out;
  int i;

  string = g_string_new("");
  logregex_refresh(LOG_DIRECTORY);

  for (i=0; errlogspec[i].path; i++) {
    int logcolor = check_log(string, i, color);
    if (logcolor > *color) *color = logcolor;
  }
  out = fopen(STATFILE, "w");
  if (out) {
    for (i=0; errlogspec[i].path; i++) {
      fprintf(out, "%s %Ld\n", errlogspec[i].path, errlogspec[i].offset);
    }
    fclose(out);
  }
  return(string);
}