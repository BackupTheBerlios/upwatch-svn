#include "config.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>

#include <generic.h>
#include <st.h>
#include "uw_send.h"

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

int online;

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
  int ct = 0;
  char **input;
  char **host, **port, **user, **pwd, **thr;
  int i = 0;

  hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, queue_free);

  if (!HAVE_OPT(THREADS)) {
    fprintf(stderr, "missing threads option\n");
    return 0;
  }
  if (!HAVE_OPT(INPUT)) {
    fprintf(stderr, "missing input option\n");
    return 0;
  }

  ct    = STACKCT_OPT( INPUT );
  input = STACKLST_OPT(INPUT);
  host  = STACKLST_OPT(HOST);
  port  = STACKLST_OPT(PORT);
  user  = STACKLST_OPT(UWUSER);
  pwd   = STACKLST_OPT(UWPASSWD);
  thr   = STACKLST_OPT(THREADS);

  // read in options for queuing
  for (i=0; input[i] && i < ct && i < 4; i++) {
    struct q_info *q = g_hash_table_lookup(hash, input[i]);
    char buf[256];

    if (q == NULL) {
      q = g_malloc0(sizeof(struct q_info));
      q->name = strdup(input[i]);
      g_hash_table_insert(hash, strdup(input[i]), q);
    }

    if (!host || !host[i]) {
      fprintf(stderr, "missing host option for input %s\n", input[i]);
      return 0;
    } else if (strcmp(host[i], "none") == 0) {
      g_hash_table_remove(hash, host[i]);
      continue;
    }
    if (!port || !port[i]) {
      fprintf(stderr, "missing port option for input queue %s\n", input[i]);
      return 0;
    }
    if (!user || !user[i]) {
      fprintf(stderr, "missing uwuser option for input queue %s\n", input[i]);
      return 0;
    }
    if (!pwd || !pwd[i]) {
      fprintf(stderr, "missing uwpasswd option for input %s\n", input[i]);
      return 0;
    }
    if (q->host) g_free(q->host);
    if (q->user) g_free(q->user);
    if (q->pwd)  g_free(q->pwd);
    q->host = strdup(host[i]);
    q->port = atoi(port[i]);
    strcpy(buf, user[i]); // preset
    if (!strchr(user[i], '@') && HAVE_OPT(REALM)) {
      sprintf(buf, "%s@%s", user[i], OPT_ARG(REALM));
    }
    q->user = strdup(buf);
    q->pwd = strdup(pwd[i]);
    q->maxthreads = thr[i] ? atoi(thr[i]) : 1;
  }
  if (HAVE_OPT(ONCE)) {
    every = ONE_SHOT;
    daemonize = FALSE;
  } else {
    daemonize = TRUE;
    every = EVERY_5SECS;
  }
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

static void *read_queue(void *data)
{
  struct q_info *q = (struct q_info *)data;
extern int forever;
static char path[PATH_MAX];
  G_CONST_RETURN gchar *filename;
  GDir *dir;
 
  q->fatal = 0;
  //sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), q->name);
  strcpy(path, OPT_ARG(SPOOLDIR));
  strcat(path, "/");
  strcat(path, q->name);
  strcat(path, "/new");
  if (debug > 3) { LOG(LOG_DEBUG, "reading queue %s", path); }
  dir = g_dir_open (path, 0, NULL);
  while ((filename = g_dir_read_name(dir)) != NULL && !q->fatal && forever) {
    char buffer[PATH_MAX];
    struct thr_data *td;
 
    sprintf(buffer, "%s/%s", path, filename);
    td = g_malloc0(sizeof(struct thr_data));
    td->q = q;
    td->filename = strdup(buffer);
    if (st_thread_create(push, td, 0, 0) == NULL) { 
      LOG(LOG_WARNING, "couldn't create thread");
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
  thread_count--;
  return NULL;
}

static void create_q_threads(gpointer key, gpointer value, gpointer user_data)
{
  if (st_thread_create(read_queue, value, 0, 0) == NULL) { 
    LOG(LOG_WARNING, "couldn't create queue thread for %s", (char *)key);
  } else {
    if (debug > 3) { LOG(LOG_DEBUG, "created new thread"); }
    thread_count++;
  }
}

int run(void)
{
  st_usleep(1); //force context switch so timers will work
  g_hash_table_foreach(hash, create_q_threads, NULL);
  if (debug > 3) { LOG(LOG_DEBUG, "waiting for all threads to finish"); }
  while (thread_count) {
    st_usleep(10000); /* 10 ms */
  }
  if (HAVE_OPT(HANGUPSCRIPT) && online) {
    int status;

    status = system(OPT_ARG(HANGUPSCRIPT));
    if (WIFEXITED(status)) {
      if (WEXITSTATUS(status) != 0) {
        LOG(LOG_WARNING, "%s: error %d", OPT_ARG(HANGUPSCRIPT), WEXITSTATUS(status));
      }
    } else {
      LOG(LOG_ERR, "%s exited abnormally", OPT_ARG(HANGUPSCRIPT));
    }
  }
  online = FALSE;
  uw_setproctitle("sleeping");
  return 0;
}

#define TIMEOUT 10000000L

void *push(void *data)
{
  int sock;
  struct hostent *hp;
  struct sockaddr_in server;
  st_netfd_t rmt_nfd;
  struct thr_data *td = (struct thr_data *)data;
  struct q_info *q = td->q;

  if (HAVE_OPT(DIALSCRIPT) && !online) {
    int status;

    status = system(OPT_ARG(DIALSCRIPT));
    if (WIFEXITED(status)) {
      if (WEXITSTATUS(status) != 0) {
        LOG(LOG_WARNING, "%s: error %d", OPT_ARG(DIALSCRIPT), WEXITSTATUS(status));
        goto quit;
      }
    } else {
      LOG(LOG_ERR, "%s exited abnormally", OPT_ARG(DIALSCRIPT));
      goto quit;
    }
    online = TRUE;
  }

  if ((hp = gethostbyname(q->host)) == (struct hostent *) 0) {
    LOG(LOG_ERR, "can't resolve %s: %s", q->host, strerror(h_errno));
    q->fatal = 1;
    goto quit;
  } 

  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  memcpy((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);
  server.sin_port = htons(q->port);

  sock = socket( AF_INET, SOCK_STREAM, 0 );
  if (sock == -1) {
    LOG(LOG_ERR, "socket: %m");
    q->fatal = 1;
    goto quit;
  }
  uw_setproctitle("connecting to %s:%d", q->host, q->port);
  if ((rmt_nfd = st_netfd_open_socket(sock)) == NULL) {
    LOG(LOG_ERR, "st_netfd_open_socket: %m", strerror(errno));
    close(sock);
    q->fatal = 1;
    goto quit;
  } 

  if (st_connect(rmt_nfd, (struct sockaddr *)&server, sizeof(server), TIMEOUT) < 0) {
    LOG(LOG_ERR, "%s:%d: %m", q->host, q->port, strerror(errno));
    st_netfd_close(rmt_nfd);
    q->fatal = 1;
    goto quit;
  }

  if (pushto(rmt_nfd, td)) {
    LOG(LOG_INFO, "uploaded %s", td->filename); 
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
    return 1;
  }

  // expect: +OK UpWatch Acceptor vx.xx. Please login
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) { 
      LOG(LOG_WARNING, "timeout reading login request string"); 
    } else { 
      LOG(LOG_WARNING, "st_read: %m"); 
    }
    return 0;
  }
  if (debug > 3) fprintf(stderr, "%s[%u] < %s", q->host, st_netfd_fileno(rmt_nfd), buffer);
  if (buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    return 0;
  } 

  sprintf(buffer, "USER %s\n", q->user);
  uw_setproctitle("%s:%d %s", q->host, q->port, buffer);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", q->host, st_netfd_fileno(rmt_nfd), buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) { 
      LOG(LOG_WARNING, "timeout writing %s", buffer); 
    } else { 
      LOG(LOG_WARNING, "st_write: %m"); 
    }
    return 0;
  }

  // expect here: +OK Please enter password
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) { 
      LOG(LOG_WARNING, "timeout reading OK enter password"); 
    } else { 
      LOG(LOG_WARNING, "st_read: %m"); 
    }
    return 0;
  }
  if (debug > 3) fprintf(stderr, "%s[%u] < %s", q->host, st_netfd_fileno(rmt_nfd), buffer);
  if (buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    return 0;
  } 

  sprintf(buffer, "PASS %s\n", q->pwd);
  uw_setproctitle("%s:%d PASS xxxxxxxx", q->host, q->port);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", q->host, st_netfd_fileno(rmt_nfd), buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) { 
      LOG(LOG_WARNING, "timeout writing %s", buffer); 
    } else { 
      LOG(LOG_WARNING, "st_write: %m"); 
    }
    return 0;
  }

  // expect here: +OK logged in, enter command
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) { 
      LOG(LOG_WARNING, "timeout reading enter command"); 
    } else { 
      LOG(LOG_WARNING, "st_read: %m"); 
    }
    return 0;
  }
  if (debug > 3) fprintf(stderr, "%s[%u] < %s", q->host, st_netfd_fileno(rmt_nfd), buffer);
  if (buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    return 0;
  } 

  sprintf(buffer, "DATA %d %s\n", filesize, basename);
  uw_setproctitle("%s:%d %s", q->host, q->port, buffer);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", q->host, st_netfd_fileno(rmt_nfd), buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) { 
      LOG(LOG_WARNING, "timeout writing %s", buffer); 
    } else { 
      LOG(LOG_WARNING, "st_write: %m"); 
    }
    return 0;
  }

  // expect here: +OK start sending your file
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) { 
      LOG(LOG_WARNING, "timeout reading DATA response"); 
    } else { 
      LOG(LOG_WARNING, "st_read: %m"); 
    }
    return 0;
  }
  if (debug > 3) fprintf(stderr, "%s[%u] < %s", q->host, st_netfd_fileno(rmt_nfd), buffer);
  if (buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    return 0;
  } 

  if ((in = fopen(td->filename, "r")) == NULL) {
    LOG(LOG_ERR, "can't open %s", td->filename);
    return 0;
  }

  uw_setproctitle("%s:%d: UPLOADING, size=%u %s", q->host, q->port,
                filesize, td->filename);
  while ((i = fread(buffer, 1, sizeof(buffer), in)) == sizeof(buffer)) {
    //LOG(LOG_DEBUG, "read %d from input", i);
    len = st_write(rmt_nfd, buffer, i, TIMEOUT);
    if (len == -1) {
      if (errno == ETIME) { 
        LOG(LOG_WARNING, "timeout writing %s", buffer); 
      } else { 
        LOG(LOG_WARNING, "st_write: %m"); 
      }
      fclose(in);
      return 0;
    }
    //LOG(LOG_DEBUG, "written %d to output", len);
  }

  if (!feof(in)) {
    LOG(LOG_ERR, "fread: %m");
    fclose(in);
    return 0;
  }
  if (i>0 && st_write(rmt_nfd, buffer, i, TIMEOUT) != i) {
    LOG(LOG_ERR, "socket write error: %m");
    fclose(in);
    return 0;
  }
  fclose(in);

  // expect here: +OK Thank you. Enter command
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) { 
      LOG(LOG_WARNING, "timeout reading enter command"); 
    } else { 
      LOG(LOG_WARNING, "st_read: %m"); 
    }
    return 0;
  }
  if (debug > 3) fprintf(stderr, "%s[%u] < %s", q->host, st_netfd_fileno(rmt_nfd), buffer);
  if (buffer[0] != '+') {
    LOG(LOG_WARNING, buffer);
    return 0;
  } 

  sprintf(buffer, "QUIT\n");
  uw_setproctitle("%s:%d %s", q->host, q->port, buffer);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", q->host, st_netfd_fileno(rmt_nfd), buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) { 
      LOG(LOG_WARNING, "timeout writing %s", buffer); 
    } else { 
      LOG(LOG_WARNING, "st_write: %m"); 
    }
    return 0;
  }

  // expect here: +OK Nice talking to you. Bye
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) { 
      LOG(LOG_WARNING, "timeout reading QUIT response", buffer); 
    } else { 
      LOG(LOG_WARNING, "st_read: %m"); 
    }
    return 0;
  }
  if (debug > 3) fprintf(stderr, "%s[%u] < %s", q->host, st_netfd_fileno(rmt_nfd), buffer);
  return 1;
}

