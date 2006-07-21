#if !defined(__DB_H) 
#define __DB_H

#include <dbi/dbi.h>

dbi_conn open_database(const char *dbtype, const char *dbhost, const char *dbport, const char *dbname, const char *dbuser, const char *dbpasswd);
void close_database(dbi_conn conn);
dbi_result db_query(dbi_conn conn, int log_dupes, const char *qry, ...);
dbi_result db_rawquery(dbi_conn conn, int log_dupes, const char *qry);

#ifndef HAVE_LIBDBI_GET_UINT
#define dbi_result_get_uint(a,b)  dbi_result_get_ulong(a,b)
#define dbi_result_get_uint_idx(a,b)  dbi_result_get_ulong_idx(a,b)
#endif

#endif /* __DB_H */
