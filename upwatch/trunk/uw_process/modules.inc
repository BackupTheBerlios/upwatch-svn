extern module ping_module;
extern module httpget_module;
extern module iptraf_module;
extern module sysstat_module;
extern module bb_cpu_module;
extern module bb_disk_module;
extern module bb_disk_module; 
extern module bb_bgp_module;
extern module bb_conn_module;
extern module bb_dhcp_module;
extern module bb_dns_module;
extern module bb_ftp_module;
extern module bb_http_module;
extern module bb_hylafax_module;
extern module bb_istmp_module;
extern module bb_msgs_module;
extern module bb_mysql_module;
extern module bb_pop3_module;
extern module bb_procs_module;
extern module bb_radius_module;
extern module bb_smtp_module;
extern module bb_squid_module;
extern module bb_svcs_module;

module *modules[] = {
  &httpget_module,
  &iptraf_module,
  &ping_module,
  &sysstat_module,
  &bb_cpu_module,
  &bb_disk_module,
  &bb_bgp_module,
  &bb_conn_module,
  &bb_dhcp_module,
  &bb_dns_module,
  &bb_ftp_module,
  &bb_http_module,
  &bb_hylafax_module,
  &bb_istmp_module,
  &bb_msgs_module,
  &bb_mysql_module,
  &bb_pop3_module,
  &bb_procs_module,
  &bb_radius_module,
  &bb_smtp_module,
  &bb_squid_module,
  &bb_svcs_module,
  NULL
};
