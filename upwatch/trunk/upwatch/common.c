#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <generic.h>
#include <db.h>

#ifdef DMALLOC 
#include "dmalloc.h"
#endif

struct dbspec *dblist;
int dblist_cnt;

int realm_exists(char *realm)
{
  int i;

  if (!dblist) {
    return FALSE;
  }
  if (realm == NULL || realm[0] == 0) {
    if (dblist[0].dbuser[0] == '\0') return FALSE;
    return TRUE;
  }
  for (i=0; i < dblist_cnt; i++) {
    if (strcmp(dblist[i].realm, realm) == 0) {
      return TRUE;
    }
  }
  return FALSE;
}

int realm_server_by_name(char *realm, char *name)
{
  int i, id = -1;
  dbi_result result;

  if (!dblist) {
    LOG(LOG_ERR, "realm_server_by_name but no dblist found");
    return id;
  }
  if (realm == NULL || realm[0] == 0) {
    i = 0;
  } else {
    for (i=0; i < dblist_cnt; i++) {
      if (strcmp(dblist[i].realm, realm) == 0) {
        break;
      }
    }
  }
  if (i == dblist_cnt) return id;
  if (!dblist[i].db) return id;
  result = db_query(dblist[i].db, 0, dblist[i].srvrbyname, name, name, name, name, name);
  if (!result) return id;
  if (dbi_result_next_row(result)) {
    id = dbi_result_get_int_idx(result, 0);
  }
  dbi_result_free(result);
  return(id);
}

int realm_server_by_ip(char *realm, char *ip)
{
  int i, id = -1;
  dbi_result result;

  if (!dblist) {
    LOG(LOG_ERR, "realm_server_by_name but no dblist found");
    return id;
  }
  if (realm == NULL || realm[0] == 0) {
    i = 0;
  } else {
    for (i=0; i < dblist_cnt; i++) {
      if (strcmp(dblist[i].realm, realm) == 0) {
        break;
      }
    }
  }
  if (i == dblist_cnt) return id;
  if (!dblist[i].db) return id;
  result = db_query(dblist[i].db, 0, dblist[i].srvrbyip, ip, ip, ip, ip, ip);
  if (!result) return id;
  if (dbi_result_next_row(result)) {
    id = dbi_result_get_int_idx(result, 0);
  }
  dbi_result_free(result);
  return(id);
}

char *realm_server_by_id(char *realm, int id)
{
  int i;
  char *name = NULL;
  dbi_result result;

  if (!dblist) {
    LOG(LOG_ERR, "realm_server_by_name but no dblist found");
    return name;
  }
  if (realm == NULL || realm[0] == 0) {
    i = 0;
  } else {
    for (i=0; i < dblist_cnt; i++) {
      if (strcmp(dblist[i].realm, realm) == 0) {
        break;
      }
    }
  }
  if (i == dblist_cnt) return name;
  if (!dblist[i].db) return name;
  result = db_query(dblist[i].db, 0, dblist[i].srvrbyid, id, id, id, id, id);
  if (!result) return name;
  if (dbi_result_next_row(result)) {
    name = dbi_result_get_string_copy_idx(result, 0);
  }
  dbi_result_free(result);
  return(name);
}

static void init_dblist(const char *dbtype, const char *dbhost, int dbport, const char *dbname, const char *dbuser, const char *dbpasswd)
{
  database *db;
  
  db = open_database(dbtype, dbhost, dbport, dbname, dbuser, dbpasswd);
  if (db) {
    dbi_result result;
    
    if (dblist) free(dblist);
    dblist = calloc(100, sizeof(struct dbspec));
    
    result = db_query(db, 0, "select pr_realm.name, pr_realm.host, "
                             "       pr_realm.port, pr_realm.dbtype, pr_realm.dbname, "
                             "       pr_realm.dbuser, pr_realm.dbpassword "
                             "from   pr_realm "
                             "where  pr_realm.id > 1");
    if (result) {
      dblist_cnt = 0;
      while (dbi_result_next_row(result)) {
        strcpy(dblist[dblist_cnt].realm, dbi_result_get_string(result, "name"));
        strcpy(dblist[dblist_cnt].host, dbi_result_get_string_copy(result, "host"));
        dblist[dblist_cnt].port = dbi_result_get_int(result, "port");
        strcpy(dblist[dblist_cnt].dbtype, dbi_result_get_string(result, "dbtype"));
        strcpy(dblist[dblist_cnt].dbname, dbi_result_get_string(result, "dbname"));
        strcpy(dblist[dblist_cnt].dbuser, dbi_result_get_string(result, "dbuser"));
        strcpy(dblist[dblist_cnt].dbpassword, dbi_result_get_string(result, "dbpassword"));
        dblist_cnt++;
      }
      dbi_result_free(result);
    }
    close_database(db);
    LOG(LOG_INFO, "read %u realms", dblist_cnt);
  } else {
    LOG(LOG_NOTICE, "could not open %s database %s@%s as user %s", dbtype, dbname, dbhost, dbuser);
  }
}

database *open_realm(char *realm, const char *dbtype, const char *dbhost, int dbport, const char *dbname, const char *dbuser, const char *dbpasswd)
{
  int i;
  database *db;
static int call_cnt = 0;

  if (!dblist || ++call_cnt == 100) {
    call_cnt = 0;
    init_dblist(dbtype, dbhost, dbport, dbname, dbuser, dbpasswd);
    if (!dblist) {
      LOG(LOG_ERR, "open_realm but no dblist found");
      return NULL;
    }
  }
  if (realm == NULL || realm[0] == 0) {
    db = open_database(dblist[0].dbtype, dblist[0].host, dblist[0].port,
            dblist[0].dbname, dblist[0].dbuser, dblist[0].dbpassword);
    return(db);
  }

  for (i=0; i < dblist_cnt; i++) {
    if (strcmp(dblist[i].realm, realm) == 0) {
      db = open_database(dblist[i].dbtype, dblist[i].host, dblist[i].port,
              dblist[i].dbname, dblist[i].dbuser, dblist[i].dbpassword);
      return(db);
    }
  }
  return(NULL);
}

int uw_password_ok(char *user, char *passwd, const char *authquery, const char *dbtype, const char *dbhost, int dbport, const char *dbname, const char *dbuser, const char *dbpasswd)
{
  database *db;
  dbi_result result;
  char user_realm[256];
  char *realm;

  strncpy(user_realm, user, sizeof(user_realm));
  realm = strrchr(user, '@');
  if (realm) {
    *realm++ = 0;
  }
  db = open_realm(realm, dbtype, dbhost, dbport, dbname, dbuser, dbpasswd);
  if (db) {
    gchar buffer[256];

    sprintf(buffer, authquery, user, passwd);
    LOG(LOG_DEBUG, buffer);
    result = db_query(db, 1, buffer);
    if (!result || dbi_result_get_numrows(result) < 1) {
      LOG(LOG_NOTICE, "realm %s: user %s, pwd %s not found", realm, user, passwd);
      close_database(db);
      return(FALSE);
    }
    if (dbi_result_next_row(result)) {
      int id;

      id = dbi_result_get_int_idx(result, 0);
      LOG(LOG_DEBUG, "realm %s: user %s, pwd %s resulted in id %d", realm, user_realm, passwd, id);
    }
    dbi_result_free(result);
    close_database(db);
  } else {
    close_database(db);
    return(FALSE); // couldn't open database
  }
  return(TRUE);
}

