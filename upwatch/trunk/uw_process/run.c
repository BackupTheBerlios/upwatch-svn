#include "config.h"
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <netdb.h>

#include <generic.h>
#include "cmd_options.h"

extern int process_ping(char *spec, GString *remark);
extern int process_httpget(char *spec, GString *remark);

struct _probe_proc {
  char *name;
  int (*process)(char *spec, GString *remark);
} prob_proc[] = {
  { "ping",         process_ping },
  { "httpget",      process_httpget },
  { NULL,  NULL }
};

void process(gpointer data, gpointer user_data);

int init(void)
{
  daemonize = TRUE;
  every = EVERY_5SECS;
  g_thread_init(NULL);
  return(1);
}

int mystrcmp(char **a, char **b)
{
  int ret = strcmp(*a, *b);

  if (ret < 0) return(-1);
  if (ret > 0) return(1);
  return(0);
 
}

int run(void)
{
  int ret = 0;
  char path[PATH_MAX];
  G_CONST_RETURN gchar *filename;
  GDir *dir;
  GPtrArray *arr = g_ptr_array_new();
  int i;
  int files = 0;

  if (debug > 3) LOG(LOG_DEBUG, "run()");
  sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), progname);
  dir = g_dir_open (path, 0, NULL);
  while ((filename = g_dir_read_name(dir)) != NULL) {
    char buffer[PATH_MAX];

    if (open_database() != 0) { // open_database will do nothing if already open
      break;
    }
    sprintf(buffer, "%s/%s", path, filename);
    g_ptr_array_add(arr, strdup(buffer));
    files++;
  }
  g_dir_close(dir);
  if (files) {
    g_ptr_array_sort(arr, mystrcmp);
    ret = 1;
  }

  for (i=0; i < arr->len; i++) {
    //printf("%s\n", g_ptr_array_index(arr,i));
    process(g_ptr_array_index(arr,i), NULL);
  }

  g_ptr_array_free(arr, TRUE);
  close_database();
  return(ret);
}

void process(gpointer data, gpointer user_data)
{
  char *filename = (char *)data;
  FILE *in;
  int lines, i;
  struct _probe_proc *probe;
  char buffer[4096];
  char *spec, method[64];
  GString *remark;
  int probe_count = 0;

  if (debug) LOG(LOG_DEBUG, "Processing %s", filename);
  if ((in = fopen(filename, "r")) == NULL) {
    return;
  }

  while (fgets(buffer, sizeof(buffer), in) != NULL) {
    int found;

    buffer[strlen(buffer)-1] = 0;
    spec = strdup(buffer);

    // <method><lines><user><password>
    //
    remark = g_string_new("");
    lines = 1; method[0] = 0; // proper initialisation protects against mis-formatted lines
    sscanf(buffer, "%s %d", method, &lines);
    for (i=lines-1; i; i--) {
      if (fgets(buffer, 1024, in) == NULL) {
        LOG(LOG_NOTICE, "fgets(%s): %m at ftell position %d, lines=%d, i=%d", filename, ftell(in), lines, i);
        return;
      }
      remark = g_string_append(remark, buffer);
    }

    for (found = 0, probe = prob_proc; probe->name; probe++) {
      if (!strcmp(probe->name, method)) {
        found = 1;
        probe_count++;
        if ((*probe->process)(spec, remark) == 0) {
          FILE *failures;

          failures = fopen(OPT_ARG(FAILURES), "a");
          if (!failures) {
            LOG(LOG_ERR, "%s: %m", OPT_ARG(FAILURES));
          } else {
            fprintf(failures, "%s", spec);
            fwrite(remark->str, remark->len, 1, failures);
            fclose(failures);
          }
        }
        break;
      }
    }
    if (!found) {
      LOG(LOG_ERR, "can't find method: %s", spec);
    }
    free(spec);
    g_string_free (remark, TRUE);
  }
  fclose(in);
  if (debug > 1) LOG(LOG_DEBUG, "Processed %d probes", probe_count);
  if (debug > 1) LOG(LOG_DEBUG, "unlink(%s)", filename);
  unlink(filename);
  free(filename);
  return;
}


