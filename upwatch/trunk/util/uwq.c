#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <uwq_options.h>
#include "generic.h"

int main (int argc, char *argv[])
{
  int arg_ct;
  GDir *dir;
  GError *error=NULL;
  G_CONST_RETURN gchar *filename;
  char *only = NULL;

  arg_ct = optionProcess( &progOptions, argc, argv );
  argc -= arg_ct;
  argv += arg_ct;

  if (argc >= 1) {
    only = *argv;
  }

  printf("%s:\n", OPT_ARG(SPOOLDIR));
  printf("%-15s | entries | %-35s | %-35s\n", " ", "oldest", "newest");

  dir = g_dir_open (OPT_ARG(SPOOLDIR), 0, &error);
  if (dir == NULL) {
    perror(OPT_ARG(SPOOLDIR));
    return 1;
  }
  while ((filename = g_dir_read_name(dir)) != NULL) {
    char buffer[PATH_MAX];
    GDir *qdir;
    char *ctim;
    int qcount = 0;
    time_t oldest=time(NULL), newest=NULL;
    G_CONST_RETURN gchar *qfilename;

    if (filename[0] == '.') continue;  // skip hidden files
    sprintf(buffer, "%s/%s", OPT_ARG(SPOOLDIR), filename);
    if (!g_file_test(buffer, G_FILE_TEST_IS_DIR)) continue;

    sprintf(buffer, "%s/%s/new", OPT_ARG(SPOOLDIR), filename);
    if (!g_file_test(buffer, G_FILE_TEST_IS_DIR)) continue;

    if (only && strcmp(only, filename)) continue;

    qdir = g_dir_open (buffer, 0, &error);
    if (qdir == NULL) {
      perror(OPT_ARG(SPOOLDIR));
      return 1;
    }
    while ((qfilename = g_dir_read_name(qdir)) != NULL) {
      char buffer[PATH_MAX];
      struct stat st;

      if (qfilename[0] == '.') continue;  // skip hidden files
      sprintf(buffer, "%s/%s/new/%s", OPT_ARG(SPOOLDIR), filename, qfilename);
      if (!g_file_test(buffer, G_FILE_TEST_IS_REGULAR)) {
        //fprintf(stderr, %s: not a regular file\n", buffer);
        continue;
      }
      if (stat(buffer, &st)) {
        perror(buffer);
        continue;
      }
      qcount++;
      if (oldest > st.st_mtime) oldest = st.st_mtime;
      if (newest < st.st_mtime) newest = st.st_mtime;
    }
    g_dir_close(qdir);
    printf("%-15s ", filename);
    printf("| %7d ", qcount); 

    ctim = ctime(&oldest);
    ctim[strlen(ctim)-1] = 0;
    printf("| %-35s ", qcount ? ctim : "");

    ctim = ctime(&newest);
    ctim[strlen(ctim)-1] = 0;
    printf("| %-35s ", qcount ? ctim : "");

    printf("\n");
  }
  g_dir_close(dir);


  return 0;
}
