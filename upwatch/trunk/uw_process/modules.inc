extern module ping_module;
extern module httpget_module;
extern module iptraf_module;

module *modules[] = {
  &httpget_module,
  &iptraf_module,
  &ping_module,
  NULL
};
