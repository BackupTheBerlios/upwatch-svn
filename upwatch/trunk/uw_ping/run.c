#include "config.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <malloc.h>
#include <errno.h>
#include <libxml/tree.h>

#include <generic.h>
#include <uwstat.h>
#include "cmd_options.h"

struct hostinfo {
  int 			id;		/* server probe id */
  char 			*ipaddress;	/* server name */
  int			count;		/* amount of packets to send */
  int			yellow;	        /* amount to miss before turning yellow */
  int			red;	        /* amount to miss before turning red */
  struct sockaddr_in	saddr; 		/* internet address */
  int 			done; 		/* true if done with this host */
  struct timeval	last_send_time;	/* time of last packet sent */
  int			num_sent;	/* number of ping packets sent */
  int			num_recv;	/* number of pings received */
  int			max_reply;	/* longest response time (microseconds) */
  int			min_reply;	/* shortest response time (microseconds) */
  int			total_time;	/* total response time (microseconds) for avg */
  char			*msg;		/* last error message */
} hostinfo;

/* data added after ICMP header for our nefarious purposes */

typedef struct ping_data {
  int			ping_count;	/* counts up to -c count or 1 */
  struct timeval	ping_ts;	/* time sent */
} PING_DATA;

#define MIN_PING_DATA sizeof(PING_DATA)
#define MAX_IP_PACKET 65536     /* (theoretical) max IP packet size */
#define SIZE_IP_HDR 20
#define SIZE_ICMP_HDR ICMP_MINLEN   /* from ip_icmp.h */
#define MAX_PING_DATA (MAX_IP_PACKET - SIZE_IP_HDR - SIZE_ICMP_HDR)
/* sized so as to be like traditional ping */
#define DEFAULT_PING_DATA_SIZE (MIN_PING_DATA + 44)

// how many ping packets to send
#define PING_COUNT 5

/* Long names for ICMP packet types */
static char *icmp_type_str[19] = {
  "ICMP Echo Reply",        /* 0 */
  "",
  "",
  "ICMP Unreachable",       /* 3 */
  "ICMP Source Quench",     /* 4 */
  "ICMP Redirect",          /* 5 */
  "",
  "",
  "ICMP Echo",              /* 8 */
  "",
  "",
  "ICMP Time Exceeded",     /* 11 */
  "ICMP Parameter Problem", /* 12 */
  "ICMP Timestamp Request", /* 13 */
  "ICMP Timestamp Reply",   /* 14 */
  "ICMP Information Request",/* 15 */
  "ICMP Information Reply",  /* 16 */
  "ICMP Mask Request",      /* 17 */
  "ICMP Mask Reply"         /* 18 */
};

static char *icmp_unreach_str[16] = {
  "ICMP Network Unreachable",    /* 0 */
  "ICMP Host Unreachable",       /* 1 */
  "ICMP Protocol Unreachable",   /* 2 */
  "ICMP Port Unreachable",       /* 3 */
  "ICMP Unreachable (Fragmentation Needed)",      /* 4 */
  "ICMP Unreachable (Source Route Failed)"        /* 5 */
  "ICMP Unreachable (Destination Network Unknown)",                /* 6 */
  "ICMP Unreachable (Destination Host Unknown)",                   /* 7 */
  "ICMP Unreachable (Source Host Isolated)",                       /* 8 */
  "ICMP Unreachable (Communication with Network Prohibited)",      /* 9 */
  "ICMP Unreachable (Communication with Host Prohibited)",         /* 10 */
  "ICMP Unreachable (Network Unreachable For Type Of Service)",    /* 11 */
  "ICMP Unreachable (Host Unreachable For Type Of Service)",       /* 12 */
  "ICMP Unreachable (Communication Administratively Prohibited)",  /* 13 */
  "ICMP Unreachable (Host Precedence Violation)",                  /* 14 */
  "ICMP Unreachable (Precedence cutoff in effect)"                 /* 15 */
};
#define ICMP_UNREACH_MAXTYPE            15

static struct hostinfo **hosts;
static struct timezone tz;
static int our_pid;
static int sock=-1;
static int num_hosts;
static int ping_pkt_size;
static int ping_data_size = DEFAULT_PING_DATA_SIZE;
static int select_time = 100;  // 1 ms - 1000 hosts/second

static int send_ping(int id, struct hostinfo *host);
static int run_actual_probes(int count);
static int wait_for_reply();

int recvfrom_wto (int s, char *buf, int len, struct sockaddr *saddr, int timo);
int handle_random_icmp(struct icmp *p, int psize, struct sockaddr_in *addr);
static int in_cksum(u_short *p, int n);

static void exit_probes(void)
{
  if (sock != -1) close(sock);
}

int init(void)
{
  struct protoent *proto;

  every = EVERY_MINUTE;
  daemonize = TRUE;
  startsec = OPT_VALUE_BEGIN;

  if ((proto = getprotobyname("icmp")) == NULL) {
    LOG(LOG_ERR, "unknown protocol icmp");
    return(0);
  }
  sock = socket(AF_INET, SOCK_RAW, proto->p_proto);
  if (sock < 0) {
    LOG(LOG_ERR, "Can't create raw socket: %m");
    return(0);
  }
  atexit(exit_probes);

  // drop privileges
  setuid(getuid());
  if (!debug && setuid(0) == 0) {
    LOG(LOG_ERR, "must not be run as root and executable must be set-user-id root");
    return(0);
  }
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

int run(void)
{
  MYSQL *mysql;
  xmlDocPtr doc, red;
  int id = 0, redcount = 0;
  struct hostinfo **newh = NULL;
  struct tm *tm;
  time_t now = time(NULL);
  int ct  = STACKCT_OPT(OUTPUT);
  char **output = STACKLST_OPT(OUTPUT);
  int i;


  tm = gmtime(&now);
  if (debug > 0) LOG(LOG_DEBUG, "reading ping info from database");
  uw_setproctitle("reading ping info from database");
  mysql = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD), OPT_VALUE_DBCOMPRESS);
  while (mysql) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char qry[256]; 

    sprintf(qry, "SELECT pr_ping_def.id, pr_ping_def.count, pr_ping_def.yellow, " 
                 "       pr_ping_def.red, pr_ping_def.ipaddress as ip, "
                 "       pr_ping_def.freq "
                 "FROM   pr_ping_def "
                 "WHERE  pr_ping_def.id > 1 and pr_ping_def.disable <> 'yes'"
                 "       and pr_ping_def.pgroup = '%d'",
                (unsigned)OPT_VALUE_GROUPID);

    if (mysql_query(mysql, qry)) {
      LOG(LOG_ERR, "%s: %s", qry, mysql_error(mysql));
      close_database(mysql);
      break;
    }
    result = mysql_store_result(mysql);
    if (!result) {
      if (debug) LOG(LOG_DEBUG, "%s: no result", qry);
      close_database(mysql);
      break;
    }
    newh = calloc(mysql_num_rows(result), sizeof(struct hostinfo));
    while ((row = mysql_fetch_row(result))) {
      u_int ip;
      struct in_addr *ipa = (struct in_addr *)&ip;

      char *ipaddress = row[4];
      int probeid = atoi(row[0]);
      int count = atoi(row[1]);
      int yellow = atoi(row[2]);
      int red = atoi(row[3]);
      int freq = atoi(row[5]); // every # minutes

      if (tm->tm_min % freq != 0) continue;
      if ((ip = inet_addr(ipaddress)) == -1) {
        LOG(LOG_NOTICE, "illegal ip address %s", ipaddress);
        continue;
      }
      if (count < 1) count = 1;
      if (count > 30) count = 30;
      if (yellow < 0) yellow = 0;
      if (yellow > count) yellow = count;
      if (red < 0) red = 0;
      if (red > count) red = count;
      if (red < yellow) red = yellow;

      newh[id] = calloc(1, sizeof(struct hostinfo));
      newh[id]->id = probeid;
      newh[id]->count = count;
      newh[id]->yellow = yellow;
      newh[id]->red = red;
      newh[id]->saddr.sin_family = AF_INET;
      newh[id]->saddr.sin_addr = *ipa;
      newh[id]->done = 0;
      id++;
    }
    mysql_free_result(result);
    close_database(mysql);
    if (debug > 0) LOG(LOG_DEBUG, "done reading");

    newh[id] = NULL;
    num_hosts = id;
    LOG(LOG_DEBUG, "%d hosts read", num_hosts);

    if (hosts != NULL) {
      for (id=0; hosts[id]; id++) {
        if (hosts[id]->msg) {
          //printf("%s\n", hosts[id]->msg);
          free(hosts[id]->msg);
        }
        free(hosts[id]);
      }
      free(hosts);
    }
    hosts = newh; // replace with new structure
    break;
  }
  if (hosts == NULL || num_hosts <= 0) {
    LOG(LOG_ERR, mysql_error(mysql));
    LOG(LOG_ERR, "no database, no cached info - bailing out");
    return 0;
  }
  if (num_hosts == 0) {
    return 0;
  }

  uw_setproctitle("running %d probes", num_hosts);
  run_actual_probes(num_hosts); /* this runs the actual probes */
  if (debug > 0) LOG(LOG_DEBUG, "done running probes");

  now = time(NULL);
  uw_setproctitle("writing results");
  doc = UpwatchXmlDoc("result"); 
  red = UpwatchXmlDoc("result"); 
  for (id=0; hosts[id]; id++) {
    xmlDocPtr cur = doc;
    xmlNodePtr subtree, ping;
    int color;
    int missed;
    char info[1024];
    char buffer[1024];

    if (hosts[id]->msg) {
      strcpy(info, hosts[id]->msg);
      free(hosts[id]->msg);
      hosts[id]->msg = NULL;
    } else {
      info[0] = 0;
    }

    missed = hosts[id]->num_sent - hosts[id]->num_recv;
    if (missed >= hosts[id]->red) {
      color = STAT_RED;
      cur = red;
      redcount++;
    } else if (missed >= hosts[id]->yellow) {
      color = STAT_YELLOW;
    } else {
      color = STAT_GREEN;
    }
    if (missed > 0) { // append a message for packet loss
      char tmp[256];
      int sent = hosts[id]->num_sent;
      int recv = hosts[id]->num_recv;
      int loss;

      loss = missed * 100 / (sent ? sent : 1);
      sprintf(tmp, "%d packets transmitted, %d packets received, %d%% packet loss", sent, recv, loss);
      strcat(info, "\n");
      strcat(info, tmp);
    }

    ping = xmlNewChild(xmlDocGetRootElement(cur), NULL, "ping", NULL);
    sprintf(buffer, "%d", hosts[id]->id);	xmlSetProp(ping, "id", buffer);
    sprintf(buffer, "%d", (int) now); 		xmlSetProp(ping, "date", buffer);
    sprintf(buffer, "%s", inet_ntoa(hosts[id]->saddr.sin_addr)); xmlSetProp(ping, "ipaddress", buffer);
    sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60)); 
      xmlSetProp(ping, "expires", buffer);
    if (color == STAT_RED) xmlSetProp(ping, "investigate", "icmptraceroute");
    sprintf(buffer, "%d", color); 		subtree = xmlNewChild(ping, NULL, "color", buffer);
    sprintf(buffer, "%f", ((float) hosts[id]->min_reply) * 0.000001);
      subtree = xmlNewChild(ping, NULL, "min", buffer);
    sprintf(buffer, "%f", ((float) (hosts[id]->total_time / (hosts[id]->num_recv==0?1:hosts[id]->num_recv))) * 0.000001);
      subtree = xmlNewChild(ping, NULL, "avg", buffer);
    sprintf(buffer, "%f", ((float) hosts[id]->max_reply) * 0.000001);
      subtree = xmlNewChild(ping, NULL, "max", buffer);
    if (info[0]) subtree = xmlNewChild(ping, NULL, "info", info);

    // reset all counters
    hosts[id]->num_sent = 0;
    hosts[id]->num_recv = 0;
    hosts[id]->done = 0;
    memset(&hosts[id]->last_send_time, 0, sizeof(struct timeval));
    hosts[id]->max_reply = 0;
    hosts[id]->min_reply = 0;
    hosts[id]->total_time = 0;
  }
  for (i=0; i < ct; i++) {
    spool_result(OPT_ARG(SPOOLDIR), output[i], doc, NULL);
  }
  if (redcount) {
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(INVESTIGATE), red, NULL);
  }
  xmlFreeDoc(doc);
  xmlFreeDoc(red);
  return(num_hosts); // amount of items processed
}

static int run_actual_probes(int count)
{
  int id = -1;
  int done = 0;
  struct hostinfo *host;

  our_pid = getpid() & 0xFFFF;
  ping_pkt_size = ping_data_size + SIZE_ICMP_HDR;

  while (done < count) {
    struct timeval now;

    //printf("done=%d\n", done, count);
    done += wait_for_reply();	/* times out */

    if (!hosts[++id]) id = 0;	/* wrap if necessary */
    host = hosts[id];
    if (host->done) continue;

    if (host->num_sent == 0) {
      done += send_ping(id, host); /* done is incremented in case of a send error */
      continue;
    }

    gettimeofday(&now, &tz);
    //printf("diff = %d\n", timeval_diff(&now, &host->last_send_time));
    if (timeval_diff(&now, &host->last_send_time) > 10000000) { // nothing received for 10 seconds 
      host->msg = strdup("No replies received during 10 seconds");
      host->done++;
      done++;
      continue;
    }
    if (host->num_sent < host->count) {
      if (timeval_diff(&now, &host->last_send_time) >= 1000000) { // sent last ping one second ago 
        done += send_ping(id, host);
      }
    }
  }
  return(0);
}

static int send_ping(int id, struct hostinfo *host)
{
  char *buffer;
  struct icmp *icp;
  PING_DATA *pdp;
  int n, ret=0;
  char buf[1024];

  buffer = (char *) malloc ((size_t)ping_pkt_size);
  if (!buffer) {
    LOG(LOG_ERR, "out of memory");
    return(1); 
  }
  memset(buffer, 0, ping_pkt_size * sizeof(char));
  icp = (struct icmp *) buffer;

  gettimeofday(&host->last_send_time, &tz);
  icp->icmp_type = ICMP_ECHO;
  icp->icmp_code = 0;
  icp->icmp_cksum = 0;
  icp->icmp_seq = id;
  icp->icmp_id = our_pid;

  pdp = (PING_DATA *) (buffer + SIZE_ICMP_HDR);
  pdp->ping_ts = host->last_send_time;
  pdp->ping_count = host->num_sent;

  icp->icmp_cksum = in_cksum( (u_short *)icp, ping_pkt_size );

  sprintf(buf, "sending [%d] to %s", host->num_sent, inet_ntoa(host->saddr.sin_addr));
  if (debug > 2) LOG(LOG_DEBUG, buf);

  n = sendto(sock, buffer, ping_pkt_size, 0, (struct sockaddr *)&host->saddr,
                                               sizeof(struct sockaddr_in) );
  if( n < 0 || n != ping_pkt_size ) {
    host->msg = strdup(strerror(errno));
    host->done++;
    ret = 1;
  } else {
    host->num_sent++;
  }
  free(buffer);
  return(ret);
}

static int wait_for_reply()
{
  int result;
  static char buffer[4096];
  struct sockaddr_in response_addr;
  struct ip *ip;
  int hlen;
  struct icmp *icp;
  int n;
  struct hostinfo *host;
  PING_DATA *pdp;
  long this_reply;
  int this_count;
  struct timeval sent_time;
  struct timeval now;

  result = recvfrom_wto(sock, buffer,4096,
                     (struct sockaddr *)&response_addr,select_time);
  if (result < 0) { return 0; } /* timeout */

  ip = (struct ip *) buffer;
#if defined(__alpha__) && __STDC__
  /* The alpha headers are decidedly broken.
   * Using an ANSI compiler, it provides ip_vhl instead of ip_hl and
   * ip_v.  So, to get ip_hl, we mask off the bottom four bits.
   */
  hlen = (ip->ip_hl & 0x0F) << 2;
#else
  hlen = ip->ip_hl << 2;
#endif

  if (result < hlen + ICMP_MINLEN) {
    //  printf("received packet too short for ICMP (%d bytes from %s)\n",
    //         result, inet_ntoa(response_addr.sin_addr));
    return(0); /* too short */
  }
  icp = (struct icmp *)(buffer + hlen);
  if (icp->icmp_type != ICMP_ECHOREPLY) {
    /* handle some problem */
    handle_random_icmp(icp, result, &response_addr);
    return(0);
  }

  if (icp->icmp_id != our_pid) {
    return(0); /* packet received, but not the one we are looking for! */
  }

  if (icp->icmp_seq  >= (n_short)num_hosts) {
    return(0); /* packet received, don't worry about it anymore */
  }

  n = icp->icmp_seq;
  host = hosts[n];

  /* received ping is cool, so process it */
  gettimeofday(&now, &tz);
  host->num_recv++;

  pdp = (PING_DATA *)icp->icmp_data;
  sent_time = pdp->ping_ts;
  this_count = pdp->ping_count;
#ifdef DEBUG
  printf("received [%d]\n", this_count);
#endif

  this_reply = timeval_diff(&now, &sent_time);
  if (host->num_recv == 1) {
    host->max_reply=this_reply;
    host->min_reply=this_reply;
  } else {
    if (this_reply > host->max_reply) host->max_reply=this_reply;
    if (this_reply < host->min_reply) host->min_reply=this_reply;
  }
  host->total_time += this_reply;

  if (host->num_recv == host->count) {
    host->done++;
    return(1);
  }
  return(0);
}

int recvfrom_wto (int s, char *buf, int len, struct sockaddr *saddr, int timo)
{
  int nfound,slen,n;
  struct timeval to;
  fd_set readset,writeset;

  to.tv_sec  = timo/100000;
  to.tv_usec = (timo - (to.tv_sec*100000))*10;

  FD_ZERO(&readset);
  FD_ZERO(&writeset);
  FD_SET(s,&readset);
  nfound = select(s+1,&readset,&writeset,NULL,&to);
  if (nfound<0) {
    if (errno == EINTR) return(-1); // fake timeout
    LOG(LOG_ERR, "select");
    exit(1);
  }
  if (nfound==0) return -1;  /* timeout */
  slen=sizeof(struct sockaddr);
  n=recvfrom(s,buf,len,0,saddr,&slen);
  if (n<0) {
    LOG(LOG_ERR, "recvfrom");
    exit(1);
  }
  return n;
}

int handle_random_icmp(struct icmp *p, int psize, struct sockaddr_in *addr)
{
  struct icmp *sent_icmp;
  u_char *c;
  struct hostinfo *host;
  char buffer[1024];

  c = (u_char *)p;
  switch (p->icmp_type) {
  case ICMP_UNREACH:
    sent_icmp = (struct icmp *) (c + 28);
    if ((sent_icmp->icmp_type == ICMP_ECHO) &&
        (sent_icmp->icmp_id == our_pid) &&
        (sent_icmp->icmp_seq < (n_short)num_hosts)) {
      /* this is a response to a ping we sent */
      host = hosts[sent_icmp->icmp_seq];
      if (p->icmp_code > ICMP_UNREACH_MAXTYPE) {
        sprintf(buffer,
                "ICMP Unreachable (Invalid Code) from %s",
                inet_ntoa(addr->sin_addr)); 
      } else {
        sprintf(buffer, "%s from %s",
                icmp_unreach_str[p->icmp_code],
                inet_ntoa(addr->sin_addr));
      }
      if (debug > 1) LOG(LOG_DEBUG, buffer);
      host->msg = strdup(buffer);
    }
    return 1;
  case ICMP_SOURCEQUENCH:
  case ICMP_REDIRECT:
  case ICMP_TIMXCEED:
  case ICMP_PARAMPROB:
    sent_icmp = (struct icmp *) (c + 28);
    if ((sent_icmp->icmp_type = ICMP_ECHO) &&
        (sent_icmp->icmp_id = our_pid) &&
        (sent_icmp->icmp_seq < (n_short)num_hosts)) {
      /* this is most likely a response to a ping we sent */
      host = hosts[sent_icmp->icmp_seq];
      sprintf(buffer, "%s from %s",
              icmp_type_str[p->icmp_type],
              inet_ntoa(addr->sin_addr));
      if (debug > 1) LOG(LOG_DEBUG, buffer);
      host->msg = strdup(buffer);
    }
    return 2;
  /* no way to tell whether any of these are sent due to our ping */
  /* or not (shouldn't be, of course), so just discard            */
  case ICMP_TSTAMP:
  case ICMP_TSTAMPREPLY:
  case ICMP_IREQ:
  case ICMP_IREQREPLY:
  case ICMP_MASKREQ:
  case ICMP_MASKREPLY:
  default:
    return 0;
  }
}

static int in_cksum(u_short *p, int n)
{
  register u_short answer;
  register long sum = 0;
  u_short odd_byte = 0;

  while( n > 1 )  { sum += *p++; n -= 2; }

  /* mop up an odd byte, if necessary */
  if( n == 1 ) {
      *(u_char *)(&odd_byte) = *(u_char *)p;
      sum += odd_byte;
  }

  sum = (sum >> 16) + (sum & 0xffff);   /* add hi 16 to low 16 */
  sum += (sum >> 16);                   /* add carry */
  answer = ~sum;                        /* ones-complement, truncate*/
  return (answer);
}

