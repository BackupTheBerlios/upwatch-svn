#include "config.h"
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>

#include <generic.h>
#include <st.h>
#include "cmd_options.h"

struct q_info {
  char *name;     // queue name
  int maxthreads; // number of processing threads for this queue
  char *host;     // host to send to
  int port;       // port to send to
  char *user;     // username for logging in
  char *pwd;      // password
  char *file;     // name of file currently being processed
  int thread_count;// currently running threads for this queue
  int fatal;      // fatal error happened, stop all current threads and wait a little
};

struct thr_data {
  struct q_info *q;
  char *filename;
};

GHashTable *hash;
void *push(void *data);
int thread_count;
int pushto(st_netfd_t rmt_nfd, struct thr_data *td);

void queue_free(void *val)
{
  struct q_info *q = (struct q_info *)val;

  if (q->name) g_free(q->name);
  if (q->host) g_free(q->host);
  if (q->user) g_free(q->user);
  if (q->pwd)  g_free(q->pwd);
  g_free(q);
}

int init(void)
{
  int ct  = STACKCT_OPT( INPUT );
  char **input = STACKLST_OPT(INPUT);
  char **host, **port, **user, **pwd, **thr;
  int i;

  hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, queue_free);

  if (!HAVE_OPT(THREADS)) {
    fprintf(stderr, "missing threads option\n");
    return 0;
  }

  host = STACKLST_OPT(HOST);
  port = STACKLST_OPT(PORT);
  user = STACKLST_OPT(UWUSER);
  pwd  = STACKLST_OPT(UWPASSWD);
  thr  = STACKLST_OPT(THREADS);

  // read in options for queuing
  for (i=0; i < ct && i < 4; i++) {
    struct q_info *q = g_hash_table_lookup(hash, input[i]);

    if (q == NULL) {
      q = g_malloc0(sizeof(struct q_info));
      q->name = strdup(input[i]);
      g_hash_table_insert(hash, strdup(input[i]), q);
    }

    if (!host[i]) {
      fprintf(stderr, "missing host option for input %s\n", input[i]);
      return 0;
    } else if (strcmp(host[i], "none") == 0) {
      g_hash_table_remove(hash, host[i]);
      continue;
    }
    if (!port[i]) {
      fprintf(stderr, "missing port option for input queue %s\n", input[i]);
      return 0;
    }
    if (!user[i]) {
      fprintf(stderr, "missing uwuser option for input queue %s\n", input[i]);
      return 0;
    }
    if (!pwd[i]) {
      fprintf(stderr, "missing uwpasswd option for input %s\n", input[i]);
      return 0;
    }
    if (q->host) g_free(q->host);
    if (q->user) g_free(q->user);
    if (q->pwd)  g_free(q->pwd);
    q->host = strdup(host[i]);
    q->port = atoi(port[i]);
    q->user = strdup(user[i]);
    q->pwd = strdup(pwd[i]);
    q->maxthreads = thr[i] ? atoi(thr[i]) : 1;
  }
  if (HAVE_OPT(ONCE)) {
    every = ONE_SHOT;
  } else {
    daemonize = TRUE;
    every = EVERY_5SECS;
  }
  st_init();
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

static void *read_queue(void *data)
{
  struct q_info *q = (struct q_info *)data;
extern int forever;
  char path[PATH_MAX];
  G_CONST_RETURN gchar *filename;
  GDir *dir;
  int i;
 
  sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), q->name);
  if (debug > 3) LOG(LOG_DEBUG, "reading queue %s", path); 
  dir = g_dir_open (path, 0, NULL);
  while ((filename = g_dir_read_name(dir)) != NULL && !q->fatal && forever) {
    char buffer[PATH_MAX];
    struct thr_data *td;
 
    sprintf(buffer, "%s/%s", path, filename);
    td = g_malloc0(sizeof(struct thr_data));
    td->q = q;
    td->filename = strdup(buffer);
    if (st_thread_create(push, td, 0, 0) == NULL) { 
      LOG(LOG_DEBUG, "couldn't create thread");
      st_sleep(1);
    } else {
      q->thread_count++;
      thread_count++;
    }
    while (q->thread_count >= q->maxthreads) {
      st_usleep(10000); /* 10 ms */
    }
  }
  g_dir_close(dir);
  uw_setproctitle("sleeping");
  thread_count--;
  return NULL;
}

static void create_q_threads(gpointer key, gpointer value, gpointer user_data)
{
  if (st_thread_create(read_queue, value, 0, 0) == NULL) { 
    LOG(LOG_DEBUG, "couldn't create queue thread for %s", (char *)key);
  } else {
    if (debug > 3) LOG(LOG_DEBUG, "created new thread");
    thread_count++;
  }
}

int run(void)
{
  g_hash_table_foreach(hash, create_q_threads, NULL);
  if (debug > 3) LOG(LOG_DEBUG, "waiting for all threads to finish");
  while (thread_count) {
    st_usleep(10000); /* 10 ms */
  }
  return 0;
}

void *push(void *data)
{
  int sock;
  struct hostent *hp;
  struct sockaddr_in server;
  st_netfd_t rmt_nfd;
  struct thr_data *td = (struct thr_data *)data;
  struct q_info *q = td->q;

  if ((hp = gethostbyname(q->host)) == (struct hostent *) 0) {
    LOG(LOG_NOTICE, "can't resolve %s: %s", q->host, hstrerror(h_errno));
    q->fatal = 1;
    goto quit;
  } 

  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  memcpy((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);
  server.sin_port = htons(q->port);

  sock = socket( AF_INET, SOCK_STREAM, 0 );
  if (sock == -1) {
    LOG(LOG_WARNING, "socket: %m");
    q->fatal = 1;
    goto quit;
  }
  uw_setproctitle("connecting to %s:%d", q->host, q->port);
  if ((rmt_nfd = st_netfd_open_socket(sock)) == NULL) {
    LOG(LOG_NOTICE, "st_netfd_open_socket: %m", strerror(errno));
    close(sock);
    q->fatal = 1;
    goto quit;
  } 

  if (st_connect(rmt_nfd, (struct sockaddr *)&server, sizeof(server), -1) < 0) {
    LOG(LOG_NOTICE, "%s:%d: %m", q->host, q->port, strerror(errno));
    st_netfd_close(rmt_nfd);
    q->fatal = 1;
    goto quit;
  }

  if (pushto(rmt_nfd, td)) {
    if (debug) LOG(LOG_NOTICE, "uploaded %s", td->filename);
    unlink(td->filename);
  }
  st_netfd_close(rmt_nfd);

quit:
  free(td->filename);
  free(td);
  q->thread_count--;
  thread_count--;
  return NULL;
}

#define TIMEOUT 10000000L

int pushto(st_netfd_t rmt_nfd, struct thr_data *td)
{
  FILE *in;
  char buffer[BUFSIZ];
  struct stat st;
  int filesize;
  int i, len;
  struct q_info *q = td->q;
  char *basename;
  
  if ((basename = strrchr(td->filename, '/')) == NULL) {
    basename = td->filename;
  } else {
    basename++;
  }
  if (stat(td->filename, &st)) {
    LOG(LOG_WARNING, "%s: %m", td->filename);
    return 0;
  }
  filesize = (int) st.st_size;
  if (filesize == 0) {
    LOG(LOG_WARNING, "zero size: %s", td->filename);
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

  sprintf(buffer, "USER %s\n", q->user);
  uw_setproctitle("%s:%d %s", q->host, q->port, buffer);
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

  sprintf(buffer, "PASS %s\n", q->pwd);
  uw_setproctitle("%s:%d PASS xxxxxxxx", q->host, q->port);
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

  sprintf(buffer, "DATA %d %s\n", filesize, basename);
  uw_setproctitle("%s:%d %s", q->host, q->port, buffer);
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

  if ((in = fopen(td->filename, "r")) == NULL) {
    LOG(LOG_WARNING, "can't open %s", td->filename);
    return 0;
  }

  uw_setproctitle("%s:%d: UPLOADING, size=%u %s", q->host, q->port,
                filesize, td->filename);
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
  uw_setproctitle("%s:%d %s", q->host, q->port, buffer);
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

