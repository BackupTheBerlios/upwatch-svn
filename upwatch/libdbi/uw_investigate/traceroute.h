#ifndef __TRACEROUTE_H
#define __TRACEROUTE_H

#define TR_ICMP 0
#define TR_TCP  1
#define TR_UDP  2

void traceroute(gpointer data, gpointer user_data);

struct trace_info {
  int type;            // TR_ICMP|TR_TCP|TR_UDP
  char *hostname;      // hostname to trace to
  char *ipaddress;     // ipaddress to trace to
  int port;	       // port to use (if TCP or UDP)
  xmlNodePtr cur;      // XML probe result
  char *workfilename;  // must be deleted when finished
};

#endif /* __TRACEROUTE_H */
