extern module ping_module;
extern module httpget_module;
extern module iptraf_module;
extern module sysstat_module;
extern module mysql_module;
extern module pop3_module;
extern module smtp_module;
extern module tcpconnect_module;
extern module imap_module;
extern module bb_cpu_module;
extern module bb_module;
extern module postgresql_module;
extern module snmpget_module;
extern module mssql_module;
extern module errlog_module;
extern module diskfree_module;

module *modules[] = {
  &httpget_module,
  &iptraf_module,
  &ping_module,
  &sysstat_module,
  &mysql_module,
  &pop3_module,
  &smtp_module,
  &tcpconnect_module,
  &imap_module,
  &errlog_module,
  &diskfree_module,
  &bb_cpu_module,
  &bb_module,
#if HAVE_LIBPQ
  &postgresql_module,
#endif
#if HAVE_LIBSNMP
  &snmpget_module,
#endif
#if HAVE_LIBTDS
  &mssql_module,
#endif
  NULL
};
