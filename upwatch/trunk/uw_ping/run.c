#include "config.h"
#include <generic.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "uw_ping.h"
#define TIMEOUT	10000000L

struct probedef {
  int           	id;             /* unique probe id */
  int           	probeid;        /* server probe id */
  char          	*domain;        /* database domain */
  int			seen;           /* seen */
  char			*ipaddress;     /* server name */
#include "probe.def_h"
#include "../common/common.h"
#include "probe.res_h"
  struct sockaddr_in    saddr;          /* internet address */
  int                   done;           /* true if done with this host */
  struct timeval        last_send_time; /* time of last packet sent */
  int                   num_sent;       /* number of ping packets sent */
  int                   num_recv;       /* number of pings received */
  int                   max_reply;      /* longest response time (microseconds) */
  int                   min_reply;      /* shortest response time (microseconds) */
  int                   total_time;     /* total response time (microseconds) for avg */
  char                  *msg;           /* last error message */
};
GHashTable *cache;

// incremented for every run - placed in the icmp seq field
// to distinguish pings sent in a previous round from pings sent in 
// the current round
static int runid;

// increments with each packet sent in the current round
// unique for each packet, so we know it when it comes back
static int pktno;

// put into the icmp id field, distinguishes our packets from
// other pinging processes on this host
static int our_pid;

// socket id
static int sock = -1;

static void exit_probes(void)
{
  if (sock != -1) close(sock);
}

void free_probe(void *probe)
{
  struct probedef *r = (struct probedef *)probe;

  if (r->ipaddress) g_free(r->ipaddress);
  if (r->domain) g_free(r->domain);
  if (r->msg) g_free(r->msg);
  g_free(r);
}

void reset_seen(gpointer key, gpointer value, gpointer user_data)
{
  struct probedef *probe = (struct probedef *)value;

  probe->seen = 0;
}

gboolean return_seen(gpointer key, gpointer value, gpointer user_data)
{
  struct probedef *probe = (struct probedef *)value;

  if (probe->seen == 0) {
    LOG(LOG_INFO, "removed probe %s:%u from list", probe->domain, probe->probeid);
    return 1;
  }
  probe->seen = 0;
  return(0);
}

int init(void)
{
  struct protoent *proto;

  daemonize = TRUE;
  every = EVERY_MINUTE;
  startsec = OPT_VALUE_BEGIN;
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  our_pid = getpid() & 0xFFFF;

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

  return(1);
}

void refresh_database(MYSQL *mysql);
void run_actual_probes(void);
void write_results(void);

int run(void)
{
  MYSQL *mysql;

  if (runid > 32000) runid = 0;
  runid++;
  pktno = 0;

  if (!cache) {
    cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_probe);
  }
  
  LOG(LOG_INFO, "reading info from database");
  uw_setproctitle("reading info from database");
  mysql = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (mysql) {
    refresh_database(mysql);
    close_database(mysql);
  }

  if (g_hash_table_size(cache) > 0) {
    LOG(LOG_INFO, "running %d probes", g_hash_table_size(cache));
    uw_setproctitle("running %d probes", g_hash_table_size(cache));
    run_actual_probes(); /* this runs the actual probes */

    LOG(LOG_INFO, "writing results");
    uw_setproctitle("writing results");
    write_results();
  }

  return(g_hash_table_size(cache));
}

void refresh_database(MYSQL *mysql)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char qry[1024];

  sprintf(qry,  "SELECT pr_ping_def.id, pr_ping_def.domid, pr_ping_def.tblid, pr_domain.name, "
                "       pr_ping_def.ipaddress, "
                "       pr_ping_def.yellow,  pr_ping_def.red "
                "FROM   pr_ping_def, pr_domain "
                "WHERE  pr_ping_def.id > 1 and pr_ping_def.disable <> 'yes'"
                "       and pr_ping_def.pgroup = '%d' and pr_domain.id = pr_ping_def.domid",
                (unsigned)OPT_VALUE_GROUPID);

  result = my_query(mysql, 1, qry);
  if (!result) {
    return;
  }
    
  while ((row = mysql_fetch_row(result))) {
    int id;
    struct probedef *probe;

    id = atol(row[0]);
    probe = g_hash_table_lookup(cache, &id);
    if (!probe) {
      probe = g_malloc0(sizeof(struct probedef));
      probe->id = id;
      if (atoi(row[1]) > 1) {
        probe->probeid = atoi(row[2]);
        probe->domain = strdup(row[3]);
      } else {
        probe->probeid = probe->id;
      }
      probe->count = 5;
      g_hash_table_insert(cache, guintdup(id), probe);
    }

    if (probe->ipaddress) g_free(probe->ipaddress);
    probe->ipaddress = strdup(row[4]);
    probe->yellow = atof(row[5]);
    probe->red = atof(row[6]);
    if (probe->msg) g_free(probe->msg);
    probe->msg = NULL;
    probe->seen = 1;
  }
  mysql_free_result(result);
  if (mysql_errno(mysql)) {
    g_hash_table_foreach(cache, reset_seen, NULL);
  } else {
    g_hash_table_foreach_remove(cache, return_seen, NULL);
  }
}

void write_probe(gpointer key, gpointer value, gpointer user_data)
{
  xmlDocPtr doc = (xmlDocPtr) user_data;
  struct probedef *probe = (struct probedef *)value;

  xmlNodePtr subtree, ping;
  int color, missed;
  char buffer[1024];
  time_t now = time(NULL);

  missed = probe->num_sent - probe->num_recv;
  if (missed >= probe->red) {
    color = STAT_RED;
  } else if (missed >= probe->yellow) {
    color = STAT_YELLOW;
  } else {
    color = STAT_GREEN;
  }
  if (missed > 0) { // append a message for packet loss
    char tmp[256];
    int loss;

    loss = missed * 100 / (probe->num_sent ? probe->num_sent : 1);
    sprintf(tmp, "%d packets transmitted, %d packets received, %d%% packet loss\n", 
            probe->num_sent, probe->num_recv, loss);
    probe->msg = strcat_realloc(probe->msg, tmp);
  }


  ping = xmlNewChild(xmlDocGetRootElement(doc), NULL, "ping", NULL);
  if (probe->domain) {
    xmlSetProp(ping, "domain", probe->domain);
  }
  sprintf(buffer, "%d", probe->probeid);      xmlSetProp(ping, "id", buffer);
  sprintf(buffer, "%s", probe->ipaddress);    xmlSetProp(ping, "ipaddress", buffer);
  sprintf(buffer, "%d", (int) now);           xmlSetProp(ping, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60));
    xmlSetProp(ping, "expires", buffer);
  sprintf(buffer, "%d", color);               xmlSetProp(ping, "color", buffer);

  sprintf(buffer, "%f", ((float) probe->min_reply) * 0.000001);
    subtree = xmlNewChild(ping, NULL, "lowest", buffer);
  sprintf(buffer, "%f", ((float) (probe->total_time / (probe->num_recv==0?1:probe->num_recv))) * 0.000001);
    subtree = xmlNewChild(ping, NULL, "value", buffer);
  sprintf(buffer, "%f", ((float) probe->max_reply) * 0.000001);
    subtree = xmlNewChild(ping, NULL, "highest", buffer);
  if (probe->msg) {
    subtree = xmlNewTextChild(ping, NULL, "info", probe->msg);
    free(probe->msg);
    probe->msg = NULL;
  }

  // reset all counters
  probe->num_sent = 0;
  probe->num_recv = 0;
  probe->done = 0;
  memset(&probe->last_send_time, 0, sizeof(struct timeval));
  probe->max_reply = 0;
  probe->min_reply = 0;
  probe->total_time = 0;
}

void write_results(void)
{
  int ct  = STACKCT_OPT(OUTPUT);
  char **output = STACKLST_OPT(OUTPUT);
  int i;
  xmlDocPtr doc;

  doc = UpwatchXmlDoc("result", NULL);

  g_hash_table_foreach(cache, write_probe, doc);
  xmlSetDocCompressMode(doc, OPT_VALUE_COMPRESS);
  for (i=0; i < ct; i++) {
    if (strcmp(output[i], "uw_test") == 0) 
      spool_result(OPT_ARG(SPOOLDIR), output[i], doc, NULL);
  }
  xmlFreeDoc(doc);
}

GArray *pi; // stores info about every ping packet sent

typedef struct ping_info {
  int			probeid;	/* probe for which this packet was send */
  int			count;		/* counts up to -c count or 1 */
  struct timeval	ts;		/* time sent */
} PING_INFO;

typedef struct ping_data {
  int                   id;		/* send id - index into ping packet hash table */
  int                   magic;		/* magic value */
} PING_DATA;

#include <netinet/ip_icmp.h>
#define SIZE_ICMP_HDR ICMP_MINLEN   /* from ip_icmp.h */
#define PING_PACKET_SIZE  (sizeof(PING_DATA)+44+SIZE_ICMP_HDR)

static int done;

int wait_for_reply(void);
int send_ping(struct probedef *probe);
static int recvfrom_wto (int s, char *buf, int len, struct sockaddr *saddr, int timo);
static int in_cksum(u_short *p, int n);
int handle_random_icmp(struct icmp *p, int psize, struct sockaddr_in *addr);

// run for each probe definition
//
void ping_step(gpointer key, gpointer value, gpointer user_data)
{
  struct probedef *probe = (struct probedef *) value;
  struct timeval now;
  struct timezone tz;

  // first check for any replies
  done += wait_for_reply();   /* times out */

  if (probe->done) return;

  // have we sent anything yet? If not, just do it.
  if (probe->num_sent == 0) {
    done += send_ping(probe); /* done is incremented in case of a send error */
    return;
  }

  gettimeofday(&now, &tz);
  //printf("diff = %d\n", timeval_diff(&now, &host->last_send_time));
  if (timeval_diff(&now, &probe->last_send_time) > 10000000) { // nothing received for 10 seconds
    probe->msg = strcat_realloc(probe->msg, "No replies received during 10 seconds\n");
    LOG(LOG_NOTICE," %s: No replies received during 10 seconds", probe->ipaddress);
    probe->done++;
    done++;
    return;
  }

  // do we need to sent the next ping packet?
  if (probe->num_sent < probe->count) {
    if (timeval_diff(&now, &probe->last_send_time) >= 1000000) { // sent last ping one second ago
      done += send_ping(probe);
    }
  }
}

void run_actual_probes(void)
{
  done = 0;

  pi = g_array_new (FALSE, FALSE, sizeof (PING_INFO));
  while (done < g_hash_table_size(cache)) {
    g_hash_table_foreach(cache, ping_step, NULL);
    fprintf(stderr, "done=%u, size=%u\n", done, g_hash_table_size(cache));
  }
  g_array_free(pi, TRUE);
}

int send_ping(struct probedef *probe)
{
  char buffer[PING_PACKET_SIZE];
  struct icmp *icp;
  PING_DATA *pdp;
  int n, ret=0;
  char buf[1024];
  struct sockaddr_in in;

  // build the packet
  memset(buffer, 0, sizeof(buffer));
  icp = (struct icmp *) buffer;

  icp->icmp_type = ICMP_ECHO;
  icp->icmp_code = 0;
  icp->icmp_cksum = 0;
  icp->icmp_seq = runid;
  icp->icmp_id = our_pid;

  pdp = (PING_DATA *) &buffer[SIZE_ICMP_HDR];
  pdp->id = pktno;
  pdp->magic = 0x1a2b3c4d;

  icp->icmp_cksum = in_cksum( (u_short *)icp, PING_PACKET_SIZE);

  sprintf(buf, "sending [%d] to %s", probe->num_sent, probe->ipaddress);
  LOG(LOG_DEBUG, buf);

  memset(&in, 0, sizeof(in));
  in.sin_family = AF_INET;
  if (inet_aton(probe->ipaddress, &in.sin_addr)) {
    n = sendto(sock, buffer, PING_PACKET_SIZE, 0, 
               (struct sockaddr *)&in, sizeof(struct sockaddr_in) );
  } else {
    n = -1; 
  }

  if( n < 0 || n != PING_PACKET_SIZE ) {
    probe->msg = strcat_realloc(probe->msg, strerror(errno));
    probe->msg = strcat_realloc(probe->msg, "\n");
    probe->done++;
    ret = 1;
  } else {
    PING_INFO pi_item;
    struct timezone tz;

    // fill the ping info itemblock with info about this packet
    pi_item.probeid = probe->id;
    pi_item.count = ++probe->num_sent;
    gettimeofday(&pi_item.ts, &tz);
    probe->last_send_time = pi_item.ts;
    g_array_append_val(pi, pi_item);
    pktno++;
  }
  return(ret);
}

int wait_for_reply(void)
{
  int result;
  static char buffer[4096];
  struct sockaddr_in response_addr;
  struct ip *ip;
  int hlen;
  struct icmp *icp;
  struct probedef *probe;
  PING_DATA *pdp;
  PING_INFO pi_item;
  long this_reply;
  struct timeval now;
  struct timezone tz;

  result = recvfrom_wto(sock, buffer, sizeof(buffer),
                     (struct sockaddr *)&response_addr, 100); // 1 ms - 1000 hosts/second
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
    LOG(LOG_NOTICE, "short packet received");
    return(0); /* too short */
  }
  icp = (struct icmp *)(buffer + hlen);
  if (icp->icmp_type != ICMP_ECHOREPLY) {
    /* handle a nonecho packet */
    handle_random_icmp(icp, result, &response_addr);
    return(0);
  }

  if (icp->icmp_id != our_pid) {
    LOG(LOG_NOTICE, "not from us");
    return(0); /* packet received, but not the one we are looking for! */
  }

  // find the probe def belonging to this packet
  pdp = (PING_DATA *)icp->icmp_data;
  pi_item = g_array_index (pi, PING_INFO, pdp->id);

  if (icp->icmp_seq  != runid) {
    LOG(LOG_NOTICE, "received: old packet for %u", pi_item.probeid);
    return(0); /* packet ok, but from a previous round */
  }

  probe = g_hash_table_lookup(cache, &pi_item.probeid);
  if (!probe) {
    LOG(LOG_ERR, "received: id=%u, probeid=%u (not found)", pdp->id, pi_item.probeid);
    return 0; // huh? 
  }

  /* received ping is cool, so process it */
  probe->num_recv++;
  gettimeofday(&now, &tz);

#ifdef DEBUG
  printf("received [%d]\n", probe->count);
#endif
  this_reply = timeval_diff(&now, &pi_item.ts);
  if (probe->num_recv == 1) {
    probe->max_reply = this_reply;
    probe->min_reply = this_reply;
  } else {
    if (this_reply > probe->max_reply) probe->max_reply = this_reply;
    if (this_reply < probe->min_reply) probe->min_reply = this_reply;
  }
  probe->total_time += this_reply;

  if (probe->num_recv == probe->count) {
    probe->done++;
    return(1);
  }
  return(0);

}

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

int handle_random_icmp(struct icmp *p, int psize, struct sockaddr_in *addr)
{
  struct icmp *sent_icmp;
  u_char *c;
  struct probedef *probe;
  char buffer[1024];
  PING_DATA *pdp;
  PING_INFO pi_item;

  c = (u_char *)p;
  switch (p->icmp_type) {
  case ICMP_UNREACH:
    sent_icmp = (struct icmp *) (c + 28);
    if ((sent_icmp->icmp_type == ICMP_ECHO) &&
        (sent_icmp->icmp_id == our_pid) &&
        (sent_icmp->icmp_seq == runid)) {
      /* this is a response to a ping we sent */
      pdp = (PING_DATA *)sent_icmp->icmp_data;
      pi_item = g_array_index (pi, PING_INFO, pdp->id);
      probe = g_hash_table_lookup(cache, &pi_item.probeid);
      if (!probe) {
        LOG(LOG_ERR, "%s: probeid = %u, not found", icmp_type_str[p->icmp_type], pi_item.probeid);
        return 0;
      }
      if (p->icmp_code > ICMP_UNREACH_MAXTYPE) {
        sprintf(buffer,
                "%s: ICMP Unreachable (Invalid Code) from %s\n",
                probe->ipaddress, inet_ntoa(addr->sin_addr));
      } else {
        sprintf(buffer, "%s: %s from %s\n",
                probe->ipaddress, icmp_unreach_str[p->icmp_code],
                inet_ntoa(addr->sin_addr));
      }
      LOG(LOG_INFO, buffer);
      probe->msg = strcat_realloc(probe->msg, buffer);
    }
    return 1;
  case ICMP_SOURCEQUENCH:
  case ICMP_REDIRECT:
  case ICMP_TIMXCEED:
  case ICMP_PARAMPROB:
    sent_icmp = (struct icmp *) (c + 28);
    if ((sent_icmp->icmp_type = ICMP_ECHO) &&
        (sent_icmp->icmp_id = our_pid) &&
        (sent_icmp->icmp_seq == runid)) {
      /* this is a response to a ping we sent */
      pdp = (PING_DATA *)sent_icmp->icmp_data;
      pi_item = g_array_index (pi, PING_INFO, pdp->id);
      probe = g_hash_table_lookup(cache, &pi_item.probeid);
      if (!probe) {
        LOG(LOG_ERR, "%s: probeid = %u, not found\n", icmp_type_str[p->icmp_type], pi_item.probeid);
        return 0;
      }
      sprintf(buffer, "%s: %s from %s\n",
              probe->ipaddress, icmp_type_str[p->icmp_type],
              inet_ntoa(addr->sin_addr));
      LOG(LOG_INFO, buffer);
      probe->msg = strcat_realloc(probe->msg, buffer);
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
    sprintf(buffer, "%s from %s",
            icmp_type_str[p->icmp_type],
            inet_ntoa(addr->sin_addr));
    LOG(LOG_INFO, buffer);
    return 0;
  }
}

static int recvfrom_wto (int s, char *buf, int len, struct sockaddr *saddr, int timo)
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
    LOG(LOG_ERR, "select: %m");
    exit(1);
  }
  if (nfound==0) return -1;  /* timeout */
  slen=sizeof(struct sockaddr);
  n=recvfrom(s,buf,len,0,saddr,&slen);
  if (n<0) {
    LOG(LOG_ERR, "recvfrom: %m");
    exit(1);
  }
  return n;
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

