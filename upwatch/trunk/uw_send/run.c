#include "config.h"
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <generic.h>
#include "cmd_options.h"

int push(gpointer data, gpointer user_data);
int pushto(int sock, char *filename);

int init(void)
{
  daemonize = TRUE;
  every = EVERY_5SECS;
  g_thread_init(NULL);
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

int run(void)
{
  int count = 0;
  char path[PATH_MAX];
  G_CONST_RETURN gchar *filename;
  GDir *dir;

  sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), OPT_ARG(INPUT));
  dir = g_dir_open (path, 0, NULL);
  while ((filename = g_dir_read_name(dir)) != NULL) {
    char buffer[PATH_MAX];

    sprintf(buffer, "%s/%s", path, filename);
    if (!push(strdup(buffer), NULL)) {
      break;
    }
    count++;
  }
  g_dir_close(dir);
  return(count);
}

int push(gpointer data, gpointer user_data)
{
  int sock;
  struct hostent *hp;
  struct sockaddr_in server;
  char *filename = (char *)data;

  if ((hp = gethostbyname(OPT_ARG(HOST))) == (struct hostent *) 0) {
    LOG(LOG_NOTICE, "can't resolve %s: %s", OPT_ARG(HOST), hstrerror(h_errno));
    free(filename);
    return 0;
  } 
  sock = socket( AF_INET, SOCK_STREAM, 0 );
  if (sock == -1) {
    LOG(LOG_WARNING, "socket: %m");
    free(filename);
    return 0;
  }
  server.sin_family = AF_INET;
  memcpy((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);
  server.sin_port = htons(OPT_VALUE_PORT);
  if (connect(sock, (struct sockaddr *) &server, sizeof server) == -1) {
    close(sock);
    LOG(LOG_WARNING, "connect: %m");
    free(filename);
    return 0;
  }
  if (pushto(sock, filename)) {
    if (debug > 1) LOG(LOG_NOTICE, "uploaded %s", filename);
    unlink(filename);
  }
  free(filename);
  close(sock);
  return 1;
}

int pushto(int sock, char *filename)
{
  FILE *in, *out;
  char buffer[BUFSIZ];
  
  if ((in = fopen(filename, "r")) == NULL) {
    LOG(LOG_WARNING, "can't open %s", filename);
    return(0);
  }
  if ((out = fdopen (sock, "r+")) == NULL) {
    LOG(LOG_WARNING, "can't fdopen socket");
    fclose(in);
    return(0);
  }
  fgets(buffer, sizeof(buffer), out);
  //fprintf(stderr, "%s", buffer);
  fprintf(out, "USER %s\n", OPT_ARG(UWUSER));
  if (fgets(buffer, sizeof(buffer), out) == NULL || buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    fclose(in);
    fclose(out);
    return(0);
  }
  //fprintf(stderr, "%s", buffer);
  fprintf(out, "PASS %s\n", OPT_ARG(UWPASSWD));
  if (fgets(buffer, sizeof(buffer), out) == NULL || buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    fclose(in);
    fclose(out);
    return(0);
  }
  //fprintf(stderr, "%s", buffer);
  while (fgets(buffer, sizeof(buffer), in)) {
    if (fprintf(out, "%s", buffer) == 0) {
      LOG(LOG_WARNING, "socket write error");
      fclose(in);
      fclose(out);
      return(0);
    }
  }
  fclose(in);
  fprintf(out, ".\n");
  if (fgets(buffer, sizeof(buffer), out) == NULL || buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    fclose(in);
    fclose(out);
    return(0);
  }
  return(fclose(out) == 0);
}

