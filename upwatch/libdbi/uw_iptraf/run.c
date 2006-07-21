#include "generic.h"
#include "uw_iptraf.h"

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
struct ipnetw extignore[256];
int count_extignore;

void writeXMLresult(struct ipnetw *ipnets, int count_ipnets)
{
  xmlDocPtr doc;
  time_t now;
  struct ipnetw *net;
  int ct  = STACKCT_OPT(OUTPUT);
  char **output = STACKLST_OPT(OUTPUT);
  int i;

  doc = UpwatchXmlDoc("result", NULL);

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
      if (HAVE_OPT(DOMAIN)) {
        xmlSetProp(iptraf, "domain", OPT_ARG(DOMAIN));
      }
      if (HAVE_OPT(REALM)) {
        xmlSetProp(iptraf, "realm", OPT_ARG(REALM));
      }
      sprintf(buffer, "%s", inet_ntoa(ip));       xmlSetProp(iptraf, "ipaddress", buffer);
      sprintf(buffer, "%u", (int) now);           xmlSetProp(iptraf, "date", buffer);
      sprintf(buffer, "%u", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60)); 
        xmlSetProp(iptraf, "expires", buffer);
      sprintf(buffer, "%u", (int) OPT_VALUE_INTERVAL); 
        xmlSetProp(iptraf, "interval", buffer);
//      if (net->count[j].in || net->count[j].out) {
        sprintf(buffer, "%u", net->count[j].in);    xmlNewChild(iptraf, NULL, "incoming", buffer);
        sprintf(buffer, "%u", net->count[j].out);   xmlNewChild(iptraf, NULL, "outgoing", buffer);
//      }
    }
    free(net->count);
  }
  free(ipnets);
  xmlSetDocCompressMode(doc, OPT_VALUE_COMPRESS);
  for (i=0; i < ct; i++) {
    spool_result(OPT_ARG(SPOOLDIR), output[i], doc, NULL);
  }
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

    for (i=0; i < OPT_VALUE_INTERVAL; i++) { // wait some seconds
      sleep(1);
      if (!forever)  {
        return(NULL);
      }
    }

    newnet = malloc(ct * sizeof(struct ipnetw));
    for (net = newnet, i = ct; i; i--, net++, pn++) {
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
      net->network = ntohl(addr.s_addr);           // network base address
      net->mask = 0xFFFFFFFF << (32 - width);      // mask
      net->size = 0x1 << (32 - width);             // size
      net->count = calloc(net->size, sizeof(struct iocount));
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
  }  
  return(NULL);
}

int init(void)
{
  if (!HAVE_OPT(OUTPUT)) {
    LOG(LOG_ERR, "missing output option");
    return 0;
  }
  if (HAVE_OPT(EXTIGNORE)) {
    int     ct  = STACKCT_OPT(EXTIGNORE);
    char**  pn = STACKLST_OPT(EXTIGNORE);
    int i;

    if (ct >= 255) { LOG(LOG_ERR, "Too many extignore statements, only 255 are supported"); }
    for (i = ct; i; i--) {
      char network[256];
      int width;
      struct in_addr addr;
      char *p;

      p = strchr(*pn, '/');
      if (!p) {
        LOG(LOG_NOTICE, "extignore %s: no slash - skipped", *pn);
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
      extignore[i].network = ntohl(addr.s_addr);
      extignore[i].mask = 0xFFFFFFFF << (32 - width);
      extignore[i].size = 0x1 << (32 - width);
      pn++;
      count_extignore++;
    }
  }
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
  struct ethhdr *et = (struct ethhdr *) packet;
  register struct ip *ip = (struct ip *) (packet + sizeof(struct ethhdr));
  uint32_t srcaddr;
  uint32_t dstaddr;
  register struct ipnetw *srcnet;
  register struct ipnetw *dstnet;
  struct ipnetw *srcign;
  struct ipnetw *dstign;
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
  srcaddr = ntohl(ip->ip_src.s_addr);
  // check if the source address is an internal network
  for (srcnet = ipnets, i = count_ipnets; i && srcnet; i--, srcnet++) {
    if ((srcaddr & srcnet->mask) != srcnet->network) { 
      //printf("%x & %x != %x\n", srcaddr, srcnet->mask,  srcnet->network);
      continue; // current ip address in this network?
    }
    if ((srcaddr & ~srcnet->mask) > srcnet->size) {
      //printf("%lx & ~%lx > %x\n", srcaddr, srcnet->mask,  srcnet->size);
      continue;    // superfluous check?
    }
    break;
  }
  if (!i) {
    srcnet = NULL; // src addr not in internal network
  }

  // now check if the source address is in the ignore list
  for (srcign = extignore, i = count_extignore; i && srcign; i--, srcign++) {
    if ((srcaddr & srcign->mask) != srcign->network) { 
      //printf("%x & %x != %x\n", srcaddr, net->mask,  net->network);
      continue; // current ip address in this network?
    }
    if ((srcaddr & ~srcign->mask) > srcign->size) {
      //printf("%lx & ~%lx > %x\n", srcaddr, srcign->mask,  srcign->size);
      continue;    // superfluous check?
    }
    break;
  }
  if (!i) {
    srcign = NULL; // src addr not in ignore list
  }

  // check if the destination address is an internal network
  dstaddr = ntohl(ip->ip_dst.s_addr);
  for (dstnet = ipnets, i = count_ipnets; i && dstnet; i--, dstnet++) {
    if ((dstaddr & dstnet->mask) != dstnet->network) { 
      //printf("%x & %x != %x\n", dstaddr, dstnet->mask,  dstnet->network);
      continue; // current ip address in this network?
    }
    if ((dstaddr & ~dstnet->mask) > dstnet->size) {
      //printf("%lx & ~%lx > %x\n", dstaddr, dstnet->mask,  dstnet->size);
      continue;    // superfluous check?
    }
    break;
  }
  if (!i) {
    dstnet = NULL; // dst addr not in internal network
  }
  // now check if the destination address is in the ignore list
  for (dstign = extignore, i = count_extignore; i && dstign; i--, dstign++) {
    if ((dstaddr & dstign->mask) != dstign->network) { 
      //printf("%x & %x != %x\n", dstaddr, net->mask,  net->network);
      continue; // current ip address in this network?
    }
    if ((dstaddr & ~dstign->mask) > dstign->size) {
      //printf("%lx & ~%lx > %x\n", dstaddr, dstign->mask,  dstign->size);
      continue;    // superfluous check?
    }
    break;
  }
  if (!i) {
    dstign = NULL; // destination addr not in ignore list
  }

  if (srcnet && !dstign && !dstnet) { // if source is internal and destination isn't (and isn't ignored)
    srcnet->count[0].out += ntohs(ip->ip_len);  // the network address itself holds total counters
    srcnet->count[srcaddr & ~srcnet->mask].out += ntohs(ip->ip_len);
  }

  if (dstnet && !srcign && !srcnet) { // if destination is internal and source isn't and isn't ignored
    dstnet->count[0].in += ntohs(ip->ip_len); // the network address itself holds total counters
    dstnet->count[dstaddr & ~dstnet->mask].in += ntohs(ip->ip_len);
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

