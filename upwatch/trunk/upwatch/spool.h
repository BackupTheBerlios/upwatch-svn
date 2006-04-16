void *spool_open(const char *basedir, const char *target, char *basename);
int spool_printf(void *sp_info, char *fmt, ...);
int spool_write(void *sp_info, char *buffer, int len);
int spool_close(void *sp_info, int complete);
char *spool_tmpfilename(void *sp_info);
char *spool_targfilename(void *sp_info);

int spool_result(const char *basedir, const char *target, xmlDocPtr doc, char **targetname);
