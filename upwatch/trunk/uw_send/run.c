#include "config.h"
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>

#include <generic.h>
#include <st.h>
#include "cmd_options.h"

void *push(void *data);
int thread_count;
int pushto(st_netfd_t rmt_nfd, char *filename);

int init(void)
{
  struct hostent *hp;

  if ((hp = gethostbyname(OPT_ARG(HOST))) == (struct hostent *) 0) {
    LOG(LOG_NOTICE, "can't resolve %s: %s", OPT_ARG(HOST), hstrerror(h_errno));
    return 0;
  } 
  daemonize = TRUE;
  every = EVERY_5SECS;
  st_init();
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
    if (st_thread_create(push, strdup(buffer), 0, 0) == NULL) { 
      LOG(LOG_DEBUG, "couldn't create thread");
    } else {
      thread_count++;
    }
    while (thread_count) {
      st_usleep(10000); /* 10 ms */
    }
    count++;
  }
  g_dir_close(dir);
  return(count);
}

void *push(void *data)
{
  int sock;
  struct hostent *hp;
  struct sockaddr_in server;
  st_netfd_t rmt_nfd;
  char *filename = (char *)data;

  if ((hp = gethostbyname(OPT_ARG(HOST))) == (struct hostent *) 0) {
    LOG(LOG_NOTICE, "can't resolve %s: %s", OPT_ARG(HOST), hstrerror(h_errno));
    free(filename);
    thread_count--;
    return NULL;
  } 

  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  memcpy((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);
  server.sin_port = htons(OPT_VALUE_PORT);

  sock = socket( AF_INET, SOCK_STREAM, 0 );
  if (sock == -1) {
    LOG(LOG_WARNING, "socket: %m");
    free(filename);
    thread_count--;
    return NULL;
  }
  uw_setproctitle("connecting to %s:%d", OPT_ARG(HOST), OPT_VALUE_PORT);
  if ((rmt_nfd = st_netfd_open_socket(sock)) == NULL) {
    LOG(LOG_NOTICE, "st_netfd_open_socket: %m", strerror(errno));
    free(filename);
    thread_count--;
    return NULL;
  } 

  if (st_connect(rmt_nfd, (struct sockaddr *)&server, sizeof(server), -1) < 0) {
    LOG(LOG_NOTICE, "st_netfd_open_socket: %m", strerror(errno));
    st_netfd_close(rmt_nfd);
    free(filename);
    thread_count--;
    return NULL;
  }

  if (pushto(rmt_nfd, filename)) {
    if (debug > 1) LOG(LOG_NOTICE, "uploaded %s", filename);
    unlink(filename);
  }
  free(filename);
  st_netfd_close(rmt_nfd);
  thread_count--;
  return NULL;
}

#define TIMEOUT 10000000L

int pushto(st_netfd_t rmt_nfd, char *filename)
{
  FILE *in;
  char buffer[BUFSIZ];
  struct stat st;
  int filesize;
  int i, len;
  
  if (stat(filename, &st)) {
    LOG(LOG_WARNING, "%s: %m", filename);
    return 0;
  }
  filesize = (int) st.st_size;
  if (filesize == 0) {
    LOG(LOG_WARNING, "zero size: %s", filename);
    return(1);
  }

  // expect: +OK UpWatch Acceptor v0.3. Please login
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == ETIME) {
    LOG(LOG_WARNING, "timeout on greeting string");
    return 0;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    return 0;
  } 

  sprintf(buffer, "USER %s\n", OPT_ARG(UWUSER));
  uw_setproctitle("%s:%d %s", OPT_ARG(HOST), OPT_VALUE_PORT, buffer);
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == ETIME) {
    LOG(LOG_WARNING, "timeout on greeting string");
    return 0;
  }
  if (len == -1) {
    LOG(LOG_WARNING, "%m");
    return 0;
  }

  // expect here: +OK Please enter password
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == ETIME) {
    LOG(LOG_WARNING, "timeout on USER response");
    return 0;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    return 0;
  } 

  sprintf(buffer, "PASS %s\n", OPT_ARG(UWPASSWD));
  uw_setproctitle("%s:%d %s", OPT_ARG(HOST), OPT_VALUE_PORT, buffer);
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == ETIME) {
    LOG(LOG_WARNING, "timeout on greeting string");
    return 0;
  }
  if (len == -1) {
    LOG(LOG_WARNING, "%m");
    return 0;
  }

  // expect here: +OK logged in, enter command
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == ETIME) {
    LOG(LOG_WARNING, "timeout on PASS response");
    return 0;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    return 0;
  } 

  sprintf(buffer, "DATA %d\n", filesize);
  uw_setproctitle("%s:%d %s", OPT_ARG(HOST), OPT_VALUE_PORT, buffer);
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == ETIME) {
    LOG(LOG_WARNING, "timeout on greeting string");
    return 0;
  }
  if (len == -1) {
    LOG(LOG_WARNING, "%m");
    return 0;
  }

  // expect here: +OK start sending your file
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == ETIME) {
    LOG(LOG_WARNING, "timeout on DATA response");
    return 0;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    return 0;
  } 

  if ((in = fopen(filename, "r")) == NULL) {
    LOG(LOG_WARNING, "can't open %s", filename);
    return 0;
  }

  uw_setproctitle("%s:%d: UPLOADING, size=%u %s", OPT_ARG(HOST), OPT_VALUE_PORT,
                filesize, filename);
  while ((i = fread(buffer, 1, sizeof(buffer), in)) == sizeof(buffer)) {
    //LOG(LOG_DEBUG, "read %d from input", i);
    len = st_write(rmt_nfd, buffer, i, TIMEOUT);
    if (len == ETIME) {
      LOG(LOG_WARNING, "timeout on greeting string");
    fclose(in);
      return 0;
    }
    if (len == -1) {
      LOG(LOG_WARNING, "%m");
      fclose(in);
      return 0;
    }
    //LOG(LOG_DEBUG, "written %d to output", len);
  }

  if (!feof(in)) {
    LOG(LOG_WARNING, "fread: %m");
    fclose(in);
    return 0;
  }
  if (i>0 && st_write(rmt_nfd, buffer, i, TIMEOUT) != i) {
    LOG(LOG_WARNING, "socket write error: %m");
    fclose(in);
    return 0;
  }
  fclose(in);

  // expect here: +OK Thank you. Enter command
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == ETIME) {
    LOG(LOG_WARNING, "timeout on DATA response");
    return 0;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    return 0;
  } 

  sprintf(buffer, "QUIT\n");
  uw_setproctitle("%s:%d %s", OPT_ARG(HOST), OPT_VALUE_PORT, buffer);
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == ETIME) {
    LOG(LOG_WARNING, "timeout on greeting string");
    return 0;
  } 
  if (len == -1) {
    LOG(LOG_WARNING, "%m");
    return 0;
  }

  // expect here: +OK Nice talking to you. Bye
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == ETIME) {
    LOG(LOG_WARNING, "timeout on QUIT response");
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  return 1;
}

