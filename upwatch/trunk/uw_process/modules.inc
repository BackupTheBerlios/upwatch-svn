extern module ping_module;
extern module httpget_module;
extern module iptraf_module;
extern module sysstat_module;
extern module mysql_module;
extern module pop3_module;
extern module mssql_module;
extern module postgresql_module;
extern module snmpget_module;
extern module imap_module;
extern module bb_generic_module;
extern module bb_cpu_module;

module *modules[] = {
  &httpget_module,
  &iptraf_module,
  &ping_module,
  &sysstat_module,
  &mysql_module,
  &pop3_module,
  &mssql_module,
  &postgresql_module,
  &snmpget_module,
  &imap_module,
  &bb_generic_module,
  &bb_cpu_module,
  NULL
};
