#include "config.h"
#include <stdarg.h>
#include <generic.h>

#ifdef DMALLOC 
#include "dmalloc.h"
#endif

void close_database(database *db)
{
  if (db && db->conn) {
    if (db->conn) dbi_conn_close(db->conn);
    free(db);
  }
  return;
}

database *open_database(const char *dbtype, const char *dbhost, int dbport, const char *dbname, const char *dbuser, const char *dbpasswd)
{
  database *db = calloc(1, sizeof(database));
static initialized = FALSE;

  strncpy(db->driver, dbtype, sizeof(db->driver));
  strncpy(db->host, dbhost, sizeof(db->host));
  sprintf(db->port, "%d", dbport);
  strncpy(db->name, dbname, sizeof(db->name));
  strncpy(db->username, dbuser, sizeof(db->username));
  strncpy(db->password, dbpasswd, sizeof(db->password));

  if (!initialized) {
    int c = dbi_initialize(NULL);
    if (c < 0) {
      LOG(LOG_ERR, "Unable to initialize libdbi! Make sure you specified a valid driver directory.\n");
      dbi_shutdown();
      return NULL;
    } else if (c == 0) {
      LOG(LOG_ERR, "Initialized libdbi, but no drivers were found!\n");
      dbi_shutdown();
      return NULL;
    }
  }
  initialized = TRUE;
  return(db);
}

dbi_result _db_rawquery(const char *file, int line, database *db, int log_dupes, char *qry)
{
  dbi_result result;
  const char *errmsg;
  int tries = 0;

  if (debug > 4) {
    LOGRAW(LOG_DEBUG, qry);
  }

  if (!dbi_conn_ping(db->conn)) {
    if (db->conn) dbi_conn_close(db->conn);
    db->conn = dbi_conn_new(db->driver);
    if (db->conn == NULL) {
      perror(db->driver);
      exit(1);
    }

    dbi_conn_set_option(db->conn, "host", db->host);
    dbi_conn_set_option(db->conn, "port", db->port);
    dbi_conn_set_option(db->conn, "username", db->username);
    dbi_conn_set_option(db->conn, "password", db->password);
    dbi_conn_set_option(db->conn, "dbname", db->name);

retry:
    if (++tries == 3) exit(1);
    if (dbi_conn_connect(db->conn) < 0) {
      dbi_conn_error(db->conn, &errmsg);
      _logsrce = file;
      _logline = line;
      _LOG(LOG_ERR, "%s connection failed to %s:%s:%s %s", db->driver, db->host, db->port, db->name, errmsg);
      sleep(3);
      goto retry;
    }

    {
      char versionstring[VERSIONSTRING_LENGTH];
      dbi_driver driver;

      driver = dbi_conn_get_driver(db->conn);
      dbi_conn_get_engine_version_string(db->conn, versionstring);
      LOG(LOG_INFO, "using driver: %s, version %s, compiled %s", dbi_driver_get_filename(driver), dbi_driver_get_version(driver),
                                    dbi_driver_get_date_compiled(driver));
      LOG(LOG_INFO, "connected to %s:%s:%s, server version %s", db->host, db->port, db->name, versionstring);
    }
  }

  if (debug > 2) {
    char *src, *dst, buf[4096];

    for (dst = buf, src = qry; *src; src++, dst++) {
      if (*src == '%') *dst++ = '%';
      *dst = *src;
    }
    *dst = 0;
    LOG(LOG_INFO, buf);
  }
  result = dbi_conn_query(db->conn, qry);
  if (result == NULL) {
    int ret = dbi_conn_error(db->conn, &errmsg);
    _logsrce = file;
    _logline = line;
    _LOG(LOG_ERR, "query failed: %s (%s)", errmsg, qry);
    if (ret == DBI_ERROR_NOCONN) {
      dbi_conn_close(db->conn);
      db->conn = NULL;
      sleep(3);
      goto retry;
    }
  }
  return(result);
}

dbi_result _db_query(const char *file, int line, database *db, int log_dupes, char *fmt, ...)
{
static char qry[65536]; // max query size for us
  va_list arg;

  if (!db) return(NULL);

  va_start(arg, fmt);
  vsnprintf(qry, sizeof(qry)-1, fmt, arg);
  va_end(arg);
  return _db_rawquery(file, line, db, log_dupes, qry);
}

const char *dbi_result_get_string_default(dbi_result Result, const char *fieldname, const char *def)
{
  const char *ret = dbi_result_get_string(Result, fieldname);

  if (ret) return ret;
  return def;
}

const char *dbi_result_get_string_default_copy(dbi_result Result, const char *fieldname, const char *def)
{
  const char *ret = dbi_result_get_string_copy(Result, fieldname);

  if (ret) return ret;
  return strdup(def);
}

// MySQL: 1062: Duplicate entry '1' for key 1
// PostgreSQL: ERROR:  duplicate key violates unique constraint ...
int dbi_duplicate_entry(dbi_conn conn)
{
  const char *errmsg;

  if (dbi_conn_error(conn, &errmsg) == DBI_ERROR_NONE) {
    return FALSE;
  }
  if (strcasestr(errmsg, "duplicate")) {
    return TRUE;
  }
  return FALSE;
}

