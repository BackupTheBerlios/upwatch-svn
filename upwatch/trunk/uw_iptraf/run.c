#include "generic.h"
#include "cmd_options.h"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <pcap.h>

static void incoming_packet(u_char *user, const struct pcap_pkthdr *hdr, const u_char *packet);
static gint mypid;
extern int forever;

struct iocount {
  uint32_t in;
  uint32_t out;
};

struct ipnetw {
  uint32_t network;
  uint32_t mask;
  uint16_t size;
  struct iocount *count;
};

static GStaticMutex m_ipnets = G_STATIC_MUTEX_INIT;
struct ipnetw *ipnets;
int count_ipnets;

void writeXMLresult(struct ipnetw *ipnets, int count_ipnets)
{
  xmlDocPtr doc;
  time_t now;
  struct ipnetw *net;
  int i;

  doc = UpwatchXmlDoc("result");
  now = time(NULL);

  for (net = ipnets, i = count_ipnets; i && net; i--, net++) {
    int j;

    for (j=0; j < net->size; j++) {
      xmlDocPtr cur = doc;
      xmlNodePtr iptraf;
      char buffer[256];
      struct in_addr ip;

      ip.s_addr = htonl(net->network + j);
      iptraf = xmlNewChild(xmlDocGetRootElement(cur), NULL, "iptraf", NULL);
      sprintf(buffer, "%s", inet_ntoa(ip));  xmlSetProp(iptraf, "id", buffer);
      sprintf(buffer, "%d", (int) now);           xmlSetProp(iptraf, "date", buffer);
      sprintf(buffer, "%d", ((int)now)+(2*60));   xmlSetProp(iptraf, "expires", buffer);
      if (net->count[j].in || net->count[j].out) {
        sprintf(buffer, "%u", net->count[j].in);    xmlNewChild(iptraf, NULL, "in", buffer);
        sprintf(buffer, "%u", net->count[j].out);   xmlNewChild(iptraf, NULL, "out", buffer);
      }
    }
    free(net->count);
  }
  free(ipnets);
  spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), doc, NULL);
  xmlFreeDoc(doc);
}

gpointer iptraf_write(gpointer data)
{
extern int forever;

  while (1) {
    int i, oldcount_ipnets;
    struct ipnetw *newnet, *oldnet, *net;
    int     ct  = STACKCT_OPT( NETWORK );
    char**  pn = STACKLST_OPT( NETWORK );

    newnet = malloc(ct * sizeof(struct ipnetw));
    for (net = newnet, i = ct; i; i--, net++) {
      char network[256];
      int width;
      struct in_addr addr;
      char *p;

      p = strchr(*pn, '/');
      if (!p) { 
        LOG(LOG_NOTICE, "no slash in %s - skipped", *pn);
        continue;
      }
      memset(network, 0, sizeof(network));
      strncpy(network, *pn, p - *pn);
      width = atoi(++p);
      if (width < 8 || width > 32) { 
        LOG(LOG_NOTICE, "illegal value for width in %s", *pn);
        continue;
      }
      inet_aton(network, &addr);
      net->network = ntohl(addr.s_addr);
      net->mask = 0xFFFFFFFF << (32 - width);
      net->size = 0x1 << (32 - width);
      net->count = calloc(net->size, sizeof(struct iocount));
      pn++;
    } 
    g_static_mutex_lock (&m_ipnets);
    oldnet = ipnets;
    ipnets = newnet;
    oldcount_ipnets = count_ipnets;
    count_ipnets = ct;
    g_static_mutex_unlock (&m_ipnets);
   
    if (oldnet) {
      writeXMLresult(oldnet, oldcount_ipnets);
    }
    for (i=0; i < 60; i++) { // wait 1 minute
      sleep(1);
      if (!forever)  {
        return(NULL);
      }
    }
  }  
  return(NULL);
}

int init(void)
{
  daemonize = TRUE;
  every = ONE_SHOT;
  g_thread_init(NULL);
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return 1;
}

gpointer pcap_thread(gpointer data)
{
  char *dev = (char *) data;
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *handle;
extern int forever;                // will be set to zero by TERM signal

  handle = pcap_open_live(dev, BUFSIZ, 1, 0, errbuf); // promiscuous, no timeout
  if (!handle) {
    LOG(LOG_ERR, "open pcap device %s: %s", dev, errbuf);
    return 0;
  }
  if (debug) LOG(LOG_INFO, "capturing on %s", dev);
  free(dev);

  while (forever) {
    pcap_dispatch(handle, 1, incoming_packet, NULL);
  }
  pcap_close(handle);
  return(NULL);
}

int run(void)
{
  GError *error;
  GThread *wt;
  GThread *pt;
  char *dev;

  wt = g_thread_create(iptraf_write, NULL, TRUE, &error);
  if (wt == NULL) {
    LOG(LOG_NOTICE, "g_thread_create: %s", error);
    return 0;
  }
  mypid = getpid() & 0xffff;
  if (HAVE_OPT(INTERFACE)) {
    int     ct  = STACKCT_OPT( INTERFACE );
    char**  pn = STACKLST_OPT( INTERFACE );

    while (ct--) {
      dev = *pn++;
      pt = g_thread_create(pcap_thread, strdup(dev), 0, &error);
      if (pt == NULL) {
        LOG(LOG_NOTICE, "g_thread_create: %s", error);
        return 0;
      }
    }
  } else {
    char errbuf[PCAP_ERRBUF_SIZE];

    dev = pcap_lookupdev(errbuf);

    pt = g_thread_create(pcap_thread, strdup(dev), 0, &error);
    if (pt == NULL) {
      LOG(LOG_NOTICE, "g_thread_create: %s", error);
      return 0;
    }
  }
  g_thread_join(wt);
  return 1;
}

static void incoming_packet(u_char *user, const struct pcap_pkthdr *hdr, const u_char *packet)
{
  register struct ethhdr *et = (struct ethhdr *) packet;
  register struct ip *ip = (struct ip *) (packet + sizeof(struct ethhdr));
  register uint32_t ipaddr = ntohl(ip->ip_src.s_addr);
  register struct ipnetw *net = ipnets;
  register unsigned int i;

#if 0
static int reporter = 0;
  char src[256];
  char dst[256];
#endif

  if (ntohs(et->h_proto) != ETH_P_IP) return; // only look at IP packets
  if (ip->ip_v != 4) return;                  // and only IP version 4 (currently)

#if 0
  strcpy(src, inet_ntoa(ip->ip_src));
  strcpy(dst, inet_ntoa(ip->ip_dst));
  //printf("id = %d, ttl = %d, protocol = %d, saddr = %s, daddr = %s, len = %d ", 
  //   ip->ip_id, ip->ip_ttl, ip->ip_p, src, dst, ntohs(ip->ip_len));
  //printf("\n");
#endif

  g_static_mutex_lock (&m_ipnets);
  ipaddr = ntohl(ip->ip_src.s_addr);
  for (net = ipnets, i = count_ipnets; i && net; i--, net++) {
    if ((ipaddr & net->mask) != net->network) { 
      //printf("%x & %x != %x\n", ipaddr, net->mask,  net->network);
      continue; // current ip address in this network?
    }
    if ((ipaddr & ~net->mask) > net->size) {
      //printf("%lx & ~%lx > %x\n", ipaddr, net->mask,  net->size);
      continue;    // superfluous check?
    }
    net->count[0].out += ntohs(ip->ip_len);  // the network address itself holds total counters
    net->count[ipaddr & ~net->mask].out += ntohs(ip->ip_len);
    break;
  }

  ipaddr = ntohl(ip->ip_dst.s_addr);
  for (net = ipnets, i = count_ipnets; i && net; i--, net++) {
    if ((ipaddr & net->mask) != net->network) { 
      //printf("%x & %x != %x\n", ipaddr, net->mask,  net->network);
      continue; // current ip address in this network?
    }
    if ((ipaddr & ~net->mask) > net->size) {
      //printf("%lx & ~%lx > %x\n", ipaddr, net->mask,  net->size);
      continue;    // superfluous check?
    }
    net->count[0].in += ntohs(ip->ip_len); // the network address itself holds total counters
    net->count[ipaddr & ~net->mask].in += ntohs(ip->ip_len);
    break;
  }
#if 0
  if (++reporter > 256) {
    reporter = 0;
    for (net = ipnets, i = count_ipnets; i && net; i--, net++) {
      int j;

      for (j=0; j < net->size; j++) {
        if (net->count[j].in || net->count[j].out) {
          printf("%d: in %d, out %d\n", j, net->count[j].in, net->count[j].out);
        }
      }
      printf("\n");
    }
  }
#endif
  g_static_mutex_unlock (&m_ipnets);
}

