#include "generic.h"
#include "traceroute.h"

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
#include <libnet.h>

static char * get_ip(char *dev);
static void incoming_packet(u_char *user, const struct pcap_pkthdr *hdr, const u_char *packet);
static u_long my_ip;
static gint mypid;
extern int forever;

void traceroute(gpointer data, gpointer user_data)
{
  struct trace_info *ti = (struct trace_info *) data;
  int network;
  char *dev, errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *handle;
  struct bpf_program filter;       /* The compiled filter expression */
  char filter_app[256];            /* The filter expression */
  bpf_u_int32 mask;                /* The netmask of our sniffing device */
  bpf_u_int32 net;                 /* The IP of our sniffing device */
extern int forever;                // will be set to zero by TERM signal

  printf("type %d\n", ti->type);
  printf("hostname %s\n", ti->hostname);
  printf("ipaddress %s\n", ti->ipaddress);
  printf("port %d\n", ti->port);
  printf("workfilename %s\n\n", ti->workfilename);

  mypid = getpid() & 0xffff;
  network = libnet_open_raw_sock(IPPROTO_RAW);
  if (network == -1) {
    LOG(LOG_ERR, "libnet_open_raw_sock: %m");
    fprintf(stderr, "libnet_open_raw_sock: %m\n");
    return;
  }
//  if (HAVE_OPT(INTERFACE)) {
//    dev = OPT_ARG(INTERFACE);
//  } else {
    dev = pcap_lookupdev(errbuf);
//  }
  handle = pcap_open_live(dev, BUFSIZ, 0, 1000, errbuf); // not promiscuous, timeout 1sec
  if (!handle) {
    libnet_close_raw_sock(network);
    LOG(LOG_ERR, "open pcap device: %s", errbuf);
    fprintf(stderr, "open pcap device: %s\n", errbuf);
    return;
  }
  if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
    libnet_close_raw_sock(network);
    pcap_close(handle);
    LOG(LOG_ERR, "pcap_lookupnet: %s", errbuf);
    fprintf(stderr, "pcap_lookupnet: %s\n", errbuf);
    return;
  }
  sprintf(filter_app, "dst %s and (icmp or udp or (tcp and tcp[13] & 0x6 != 0))", get_ip(dev));
  printf("%s\n", filter_app);
  pcap_compile(handle, &filter, filter_app, 0, net);
  pcap_setfilter(handle, &filter);
  my_ip = libnet_name_resolve(get_ip(dev), LIBNET_RESOLVE);

  ////////////////////// main loop ////////////////////

  while (forever) {
    pcap_dispatch(handle, 1, incoming_packet, (void *)&network);
  }
  libnet_close_raw_sock(network);
  pcap_close(handle);

  if (ti->hostname) free(ti->hostname);
  if (ti->ipaddress) free(ti->ipaddress);
  if (ti->workfilename) free(ti->workfilename);
  free(ti);
}

static char * get_ip(char *dev)
{ 
  int fd;
  struct ifreq i;
        
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  strncpy(i.ifr_name, dev, 5); 
  ioctl(fd, SIOCGIFADDR, (int) &i);
  close(fd);
        
  return (char *)inet_ntoa(((struct sockaddr_in *) &i.ifr_addr)->sin_addr);
}       

static void incoming_packet(u_char *user, const struct pcap_pkthdr *hdr, const u_char *packet)
{
  register struct ip *ip = (struct ip *) (packet + sizeof(struct ethhdr));
  register struct tcphdr *tcphdr;
  register struct udphdr *udphdr;
  register struct icmp *icmp;
  int network = *(int *)user;
  char src[256];
  char dst[256];

/*
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ip_hl:4;               // header length 
    unsigned int ip_v:4;                // version 
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned int ip_v:4;                // version 
    unsigned int ip_hl:4;               // header length 
#endif
    u_int8_t ip_tos;                    // type of service 
    u_short ip_len;                     // total length 
    u_short ip_id;                      // identification 
    u_short ip_off;                     // fragment offset field 
#define IP_RF 0x8000                    // reserved fragment flag 
#define IP_DF 0x4000                    // dont fragment flag 
#define IP_MF 0x2000                    // more fragments flag 
#define IP_OFFMASK 0x1fff               // mask for fragmenting bits 
    u_int8_t ip_ttl;                    // time to live 
    u_int8_t ip_p;                      // protocol 
    u_short ip_sum;                     // checksum 
    struct in_addr ip_src, ip_dst;      // source and dest address 
*/
  strcpy(src, inet_ntoa(ip->ip_src));
  strcpy(dst, inet_ntoa(ip->ip_dst));
  printf("id = %d, ttl = %d, protocol = %d, saddr = %s, daddr = %s ", 
     ip->ip_id, ip->ip_ttl, ip->ip_p, src, dst);

  switch (ip->ip_p) {
  case 1:  // icmp
/*
struct icmp
{
  u_int8_t  icmp_type;  // type of message, see below 
  u_int8_t  icmp_code;  // type sub code 
  u_int16_t icmp_cksum; // ones complement checksum of struct 
  union
  {
    u_char ih_pptr;             // ICMP_PARAMPROB 
    struct in_addr ih_gwaddr;   // gateway address 
    struct ih_idseq             // echo datagram 
    {
      u_int16_t icd_id;
      u_int16_t icd_seq;
    } ih_idseq; 
    u_int32_t ih_void;
    
    // ICMP_UNREACH_NEEDFRAG -- Path MTU Discovery (RFC1191) 
    struct ih_pmtu
    {
      u_int16_t ipm_void;
      u_int16_t ipm_nextmtu;
    } ih_pmtu;
    
    struct ih_rtradv
    {
      u_int8_t irt_num_addrs;
      u_int8_t irt_wpa;
      u_int16_t irt_lifetime;
    } ih_rtradv;
  } icmp_hun;
#define icmp_pptr       icmp_hun.ih_pptr
#define icmp_gwaddr     icmp_hun.ih_gwaddr
#define icmp_id         icmp_hun.ih_idseq.icd_id
#define icmp_seq        icmp_hun.ih_idseq.icd_seq
#define icmp_void       icmp_hun.ih_void
#define icmp_pmvoid     icmp_hun.ih_pmtu.ipm_void
#define icmp_nextmtu    icmp_hun.ih_pmtu.ipm_nextmtu
#define icmp_num_addrs  icmp_hun.ih_rtradv.irt_num_addrs
#define icmp_wpa        icmp_hun.ih_rtradv.irt_wpa
#define icmp_lifetime   icmp_hun.ih_rtradv.irt_lifetime
  union
  {
    struct
    {
      u_int32_t its_otime;
      u_int32_t its_rtime;
      u_int32_t its_ttime;
    } id_ts;
    struct
    {
      struct ip idi_ip;
      // options and then 64 bits of data 
    } id_ip;
    struct icmp_ra_addr id_radv;
    u_int32_t   id_mask;
    u_int8_t    id_data[1];
  } icmp_dun;
#define icmp_otime      icmp_dun.id_ts.its_otime
#define icmp_rtime      icmp_dun.id_ts.its_rtime
#define icmp_ttime      icmp_dun.id_ts.its_ttime
#define icmp_ip         icmp_dun.id_ip.idi_ip
#define icmp_radv       icmp_dun.id_radv
#define icmp_mask       icmp_dun.id_mask
#define icmp_data       icmp_dun.id_data
};
*/
    icmp = (struct icmp *) (packet + sizeof(struct ethhdr) + (ip->ip_hl * 4));
    printf("icmp type=%d", icmp->icmp_type);
    if (icmp->icmp_type == ICMP_TIMXCEED && icmp->icmp_code == ICMP_TIMXCEED_INTRANS) {
      struct ip *iip = &icmp->icmp_ip;

      if (ntohs(iip->ip_id) != mypid) return; // not one of ours
      // at this point we have an icmp time exceeded, and it's ours.
      // find out which probe it came from. If the original packet was an icmp echo request
      // the probe number is in the payload.
      // if the original packet was an udp packet we allocated a unique source port, and linked
      // that to the probe, if the packet was a tcp packet, 
    }
    break;
  case 6:  // tcp
/*
    u_int16_t th_sport;         // source port 
    u_int16_t th_dport;         // destination port 
    tcp_seq th_seq;             // sequence number 
    tcp_seq th_ack;             // acknowledgement number 
#  if __BYTE_ORDER == __LITTLE_ENDIAN
    u_int8_t th_x2:4;           // (unused) 
    u_int8_t th_off:4;          // data offset 
#  endif
#  if __BYTE_ORDER == __BIG_ENDIAN
    u_int8_t th_off:4;          // data offset 
    u_int8_t th_x2:4;           // (unused) 
#  endif
    u_int8_t th_flags;
#  define TH_FIN        0x01
#  define TH_SYN        0x02
#  define TH_RST        0x04
#  define TH_PUSH       0x08
#  define TH_ACK        0x10
#  define TH_URG        0x20
    u_int16_t th_win;           // window 
    u_int16_t th_sum;           // checksum 
    u_int16_t th_urp;           // urgent pointer 

*/
    tcphdr = (struct tcphdr *) (packet + sizeof(struct ethhdr) + (ip->ip_hl * 4));
    printf("tcp sourceport = %d, destport = %d, seq = %x, ack = %x ", 
       ntohs(tcphdr->th_sport), ntohs(tcphdr->th_dport), tcphdr->th_seq, tcphdr->th_ack);
    break;
  case 17: // udp
/*
         u_int16_t uh_sport;           // source port 
         u_int16_t uh_dport;           // destination port 
         u_int16_t uh_ulen;            // udp length 
         u_int16_t uh_sum;             // udp checksum 

*/
    udphdr = (struct udphdr *) (packet + sizeof(struct ethhdr) + (ip->ip_hl * 4));
    printf("udp sourceport = %d, destport = %d, len = %d ", 
       ntohs(udphdr->uh_sport), ntohs(udphdr->uh_dport), ntohs(udphdr->uh_ulen));
    break;
  }
  printf("\n");
}

