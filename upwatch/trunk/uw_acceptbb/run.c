#include "config.h"
#include <generic.h>
#include <time.h>
#define _XOPEN_SOURCE /* glibc2 needs this for strptime */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#include <st.h>
#include "cmd_options.h"

#define TIMEOUT 10000000L
int thread_count;

/*
static char *chop(char *s, int i)
{
  i--;
  while (i > 0 && isspace(s[i])) {
    s[i--] = 0;
  }
  return(s);
}
*/

int init(void)
{
  daemonize = TRUE;
  every = ONE_SHOT;
  st_init();
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

void *handle_connections(void *arg);
void *writexmlfile(void *data);

int run(void)
{
  int n, sock;
  struct sockaddr_in serv_addr;
  st_netfd_t nfd;
extern int forever;

  /* Start the main loop */
  uw_setproctitle("accepting connections");
  
  /* Create server socket */
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    LOG(LOG_NOTICE, "socket: %m");
    return 0;
  }
  n = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof(n)) < 0) {
    LOG(LOG_NOTICE, "setsockopt(REUSEADDR): %m");
    close(sock);
    return 0;
  }
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(OPT_VALUE_LISTEN);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
    struct hostent *hp;
    /* not dotted-decimal */
    if ((hp = gethostbyname("0.0.0.0")) == NULL) {
      LOG(LOG_NOTICE, "0.0.0.0: %m");
      close(sock);
      return 0;
    }
    memcpy(&serv_addr.sin_addr, hp->h_addr, hp->h_length);
  }

  /* Do bind and listen */
  if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    LOG(LOG_NOTICE, "bind: %m");
    close(sock);
    return 0;
  }
  if (listen(sock, 10) < 0) {
    LOG(LOG_NOTICE, "listen: %m");
    close(sock);
    return 0;
  }

  /* Create file descriptor object from OS socket */
  if ((nfd = st_netfd_open_socket(sock)) == NULL) {
    LOG(LOG_NOTICE, "st_netfd_open_socket: %m");
    close(sock);
    return 0;
  }

  // create timeout thread: write xml file
  if (st_thread_create(writexmlfile, NULL, 0, 0) != NULL) {
    thread_count++;
  } 

  // create connection threads
  for (n = 0; n < 10; n++) {
    if (st_thread_create(handle_connections, (void *)&nfd, 0, 0) != NULL) {
      thread_count++;
    } else {
      LOG(LOG_NOTICE, "st_thread_create: %m");
    }
  }

  while (thread_count && forever) {
    st_usleep(10000);
  }
  return(1);
}

void handle_session(st_netfd_t cli_nfd, char *remotehost);
void runbb(char *cmd);

void *handle_connections(void *arg)
{
extern int forever;
  st_netfd_t srv_nfd, cli_nfd;
  struct sockaddr_in from;
  int fromlen = sizeof(from);

  srv_nfd = *(st_netfd_t *) arg;

  while (forever) {
    char *remote;
    int len;
    char buffer[BUFSIZ];

    cli_nfd = st_accept(srv_nfd, (struct sockaddr *)&from, &fromlen, -1);
    if (cli_nfd == NULL) {
      LOG(LOG_NOTICE, "st_accept: %m");
      continue;
    }
    remote = strdup(inet_ntoa(from.sin_addr));
    if (debug > 1) LOG(LOG_NOTICE, "New connection from: %s", remote);

    memset(buffer, 0, sizeof(buffer));
    len = st_read(cli_nfd, buffer, sizeof(buffer), TIMEOUT);
    if (len == ETIME) {
      LOG(LOG_WARNING, "timeout on BB report");
      continue;
    }
    if (debug > 3) fprintf(stderr, "< %s", buffer);
    if (len > 1024) {
      strcpy(&buffer[990], "\n\n... DATA TRUNCATED ...\n\n");
    } else {
      buffer[len+1] = 0;
    }
    for(;len>=0; len--) {
      if (buffer[len] & 0x80 || buffer[len] == '\r') {
        buffer[len] = ' ';
      }
    }
    runbb(buffer);
    free(remote);
    st_netfd_close(cli_nfd);
  }
  return NULL;
}


xmlDocPtr doc = NULL;

void *writexmlfile(void *data)
{
extern int forever;

  while (forever) {
    int i;

    if (debug > 2) LOG(LOG_DEBUG, "timeout function");
    if (doc) {
      int ct  = STACKCT_OPT(OUTPUT);
      char **output = STACKLST_OPT(OUTPUT);

      for (i=0; i < ct; i++) {
        spool_result(OPT_ARG(SPOOLDIR), output[i], doc, NULL);
      }
      xmlFreeDoc(doc);
      doc = NULL;
    }
    for (i=0; i < 60 && forever; i++) {
      st_sleep(1);
    }
  }
  return NULL;
}

void add_to_xml_document(char *hostname, char *probename, char *colorstr, struct tm *probedate, char *message)
{
  xmlNodePtr subtree, probe, host;
  time_t date = mktime(probedate);
  char buffer[1024];
  char ipaddress[20];
  int color = STAT_GREEN;
  struct hostent *hp;

  if (doc == NULL) {
    doc = UpwatchXmlDoc("result");
    xmlSetDocCompressMode(doc, OPT_VALUE_COMPRESS);
  }
  if (!strcmp(colorstr, "red")) {
    color = STAT_RED;
  } else if (!strcmp(colorstr, "green")) {
    color = STAT_GREEN;
  } else if (!strcmp(colorstr, "yellow")) {
    color = STAT_YELLOW;
  }

  if ((hp = gethostbyname(hostname)) != NULL) {
    struct sockaddr_in serv_addr;

    memcpy(&serv_addr.sin_addr, hp->h_addr, hp->h_length);
    strcpy(ipaddress, inet_ntoa(serv_addr.sin_addr));
  } else {
    ipaddress[0] = 0;
  }

  if (strcmp(probename, "cpu") == 0) {
    probe = xmlNewChild(xmlDocGetRootElement(doc), NULL, "bb_cpu", NULL);
  } else {
    probe = xmlNewChild(xmlDocGetRootElement(doc), NULL, "bb", NULL);
    xmlSetProp(probe, "bbname", probename);
  }

  sprintf(buffer, "%d", (int) date);          xmlSetProp(probe, "date", buffer);
  xmlSetProp(probe, "ipaddress", ipaddress);
  sprintf(buffer, "%d", ((int)date)+((unsigned)OPT_VALUE_EXPIRES*60));
  xmlSetProp(probe, "expires", buffer); // 200 minutes
  host = xmlNewChild(probe, NULL, "host", NULL);
  sprintf(buffer, "%s", hostname);     subtree = xmlNewChild(host, NULL, "hostname", buffer);
  //sprintf(buffer, "%s", inet_ntoa(hosts[id]->saddr.sin_addr));
  //  subtree = xmlNewChild(host, NULL, "ipaddress", buffer);
  sprintf(buffer, "%d", color);               subtree = xmlNewChild(probe, NULL, "color", buffer);
  if (message && *message) {
    subtree = xmlNewChild(probe, NULL, "info", message);
  }
}

void runbb(char *cmd)
{
 /*
  * IF THE REQUEST STARTS WITH "page" THEN CALL THE PAGING SCRIPT
  * IF THE REQUEST STARTS WITH "status" THEN LOG THE MESSAGE
  * IF THE REQUEST STARTS WITH "combo" THEN SET A FLAG AND PROCESS MSGS
  * IF THE REQUEST STARTS WITH "summary" THEN LOG THE MESSAGE
  * IF THE REQUEST STARTS WITH "ack" THEN PROCEED WITH AN ACKNOWLEDGEMENT
  * IF THE REQUEST STARTS WITH "enable" THEN ENABLE THE GIVEN HOSTS
  * IF THE REQUEST STARTS WITH "disable" THEN DISABLE THE GIVEN HOSTS
  * IF THE REQUEST STARTS WITH "offline" THEN SET UP THE OFFLINED HOST
  * IF THE REQUEST STARTS WITH "online" THEN SET UP THE ONLINED HOST
  * IF THE REQUEST STARTS WITH "data" THEN APPEND DATA TO THE GIVEN FILE NAME
  * IF THE REQUEST STARTS WITH "notes" THEN REPLACE THE NOTES FILE IN www/notes
  * IF THE REQUEST STARTS WITH "test" THEN DO NOTHING (MAYBE LATER DO SOMETHING)
  * OTHERWISE IGNORE IT (FOR SAFETY SAKE)
  */

  /* If received an agent message, skip that keyword */
  if (strncmp(cmd, "bbagent ", 8) == 0) {
    cmd += 8;
  }

  if (strncmp(cmd, "status", 6) == 0) {
    char *hostname;
    char *probename;
    char *color;
    struct tm probedate;
    char *message;
    char *p;

    for (p=cmd; *p; p++) {
      if (*p & 0x80) *p = ' ';
    }
    memset(&probedate, 0, sizeof(struct tm));
    cmd += 6;
    if (*cmd == '+') {      // status+<delay> format - skip for now
      while (*cmd && *cmd != ' ') cmd++;
    }
    while (*cmd && *cmd == ' ') cmd++;
    if (!*cmd) {
      LOG(LOG_DEBUG, "Illegal status message format, dot not found");
      return;
    }
    //  status ntserver5,domain,nl.disk green Mon Oct 07 14:37:54 RDT 2002 [ntserver5.domain.nl]
    //
    //
    //	      Filesystem   1K-blocks     Used    Avail Capacity  Mounted
    //	      C              4192933  1357729  2835204    32%    /FIXED/C
    // 	      D              8883912  2171416  6712496    24%    /FIXED/D
    //

    // isolate hostname
    p = strchr(cmd, '.');
    if (!p) {
      LOG(LOG_DEBUG, "Illegal status message format, dot not found");
      return;
    }
    *p = 0;
    hostname = cmd;
    cmd = ++p;
    for (p=hostname; *p; p++) if (*p == ',') *p = '.'; // replace comma's with dots

    // isolate probe
    p = strchr(cmd, ' ');
    if (!p) {
      LOG(LOG_DEBUG, "Illegal status message format, probename not found");
      return;
    }
    *p = 0;
    probename = cmd;
    cmd = ++p;

    // isolate color
    p = strchr(cmd, ' ');
    if (!p) {
      LOG(LOG_DEBUG, "Illegal status message format, color not found");
      return;
    }
    *p = 0;
    color = cmd;
    cmd = ++p;
    while (*cmd == ' ') cmd++; // skip leading spaces

    // isolate date/time
    if (debug > 1) LOG(LOG_WARNING, "datestring: %s", cmd); 
    // Mon Oct 07 14:37:54 RDT 2002
    message = strptime(cmd, "%a %b %d %T ", &probedate);
    if (!message) {
      LOG(LOG_DEBUG, "Illegal status message format, illegal time format");
      return;
    }
    message = strptime(message+4, "%Y", &probedate); // lose the timezone info - it's unusable

    //fprintf(stderr, message);
    add_to_xml_document(hostname, probename, color, &probedate, message);
    if (debug > 1) LOG(LOG_WARNING, "host: %s, probe: %s, color: %s, msg: %s, date: %s", 
		    hostname, probename, color, message, asctime(&probedate));
  } else {
    if (debug > 1) LOG(LOG_WARNING, "unknown message: %s", cmd);
  }
}
