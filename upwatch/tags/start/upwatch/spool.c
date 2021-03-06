#include "config.h"
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <generic.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/time.h>

/* write a file to a maildir format directory */

struct spoolinfo {
FILE *fd;
char *temp_filename;
char *targ_filename;
} spoolinfo;
static char *temp_filename;

static int stat_dir(char *dirname);
static char *mk_tempfile(void);
static void alarm_handler(int sig); 
static void set_handler(int sig, void (*h)());
 
void *spool_open(char *basedir, char *target)
{
  struct stat filestat;
  int count;
  int outfd;
  char buffer[BUFSIZ];
  char *outfile;
  struct spoolinfo *si = calloc(1, sizeof(struct spoolinfo));

  if (si == NULL) {
    return(NULL);
  }

  /* Step 1:  Check that the supplied directories are OK. */
  sprintf(buffer, "%s/target/tmp", target, basedir); // I know, no bounds checking..
  if (!stat_dir(buffer)) {
    LOG(LOG_ERR, "Cannot touch %s", buffer);
    free(si);
    return(NULL);
  }
  sprintf(buffer, "%s/%s/new", basedir, target);
  if (!stat_dir(buffer)) {
    LOG(LOG_ERR, "Cannot touch %s", buffer);
    free(si);
    return(NULL);
  }

  /* Step 2:  Stat the temporary file.  Wait for ENOENT as a response. */
  for(count=1;;count++) {
    /* Get the temporary filename to use now for dumping data. */
    outfile = mk_tempfile(); // returns malloc'ed string
    if (outfile == NULL) {
      free(si);
      LOG(LOG_ERR, "can not construct temp filename");
      return(NULL);
    }
    sprintf(buffer, "%s/%s/tmp/%s", basedir, target, outfile);
    if(stat(buffer,&filestat) == -1 && errno == ENOENT) {
      si->temp_filename = strdup(buffer);
      sprintf(buffer, "%s/%s/new/%s", basedir, target, outfile);
      si->targ_filename = strdup(buffer);
      free(outfile);
      break;
    }

    /* Try up to 5 times, every 2 seconds. */
    if(count == 5) {
      free(si);
      LOG(LOG_ERR, "could not stat %s", buffer);
      return(NULL);
    }

    /* Wait 2 seconds, and try again. */
    free(outfile);
    sleep(2);
  }

  /* Step 4:  Create the file tempdir/time.pid.host */
  /* Declare a handler for SIGALRM so we can time out. */
  set_handler(SIGALRM, alarm_handler);
  alarm(86400);
  outfd = open(si->temp_filename,O_WRONLY | O_EXCL | O_CREAT,0644);
  if(outfd == -1) {
    LOG(LOG_ERR, "couldn't create %s", si->temp_filename);
    free(si->temp_filename);
    free(si->targ_filename);
    free(si);
    alarm(0);
    return(NULL);
  }
  si->fd = fdopen(outfd, "w");
  if (si->fd == NULL) {
    LOG(LOG_ERR, "could not create %s", si->temp_filename);
    free(si->temp_filename);
    free(si->targ_filename);
    free(si);
    alarm(0);
    return(NULL);
  }
  return((void *)si);
}

char *spool_tmpfilename(void *sp_info)
{
  struct spoolinfo *si = (struct spoolinfo *)sp_info;

  return(si->temp_filename);
}

char *spool_targfilename(void *sp_info)
{
  struct spoolinfo *si = (struct spoolinfo *)sp_info;

  return(si->targ_filename);
}

int spool_write(void *sp_info, char *buffer, int len)
{
  struct spoolinfo *si = (struct spoolinfo *)sp_info;

  if (fwrite(buffer, len, 1, si->fd) != 1) {
    return(-1);
  }
  return(len);
}

int spool_printf(void *sp_info, char *fmt, ...)
{
  int len;
  char buffer[BUFSIZ];
  struct spoolinfo *si = (struct spoolinfo *)sp_info;
  va_list arg;

  va_start(arg, fmt);
  len = vsnprintf(buffer, BUFSIZ, fmt, arg);
  va_end(arg);

  if (fwrite(buffer, len, 1, si->fd) != 1) {
    return(-1);
  }
  return(len);
}

int spool_close(void *sp_info, int complete)
{
  int ok = 1;
  struct spoolinfo *si = (struct spoolinfo *)sp_info;

  if (!complete) { // abort spooling
    fclose(si->fd);
    ok = FALSE;
  }
  
  if (ok && fflush(si->fd) != 0) {  // flushing fails 
    fclose(si->fd);
    ok = FALSE;
  }
  if (ok && fsync(fileno(si->fd)) != 0) { // syncing fails
    fclose(si->fd);
    ok = FALSE;
  }
  if (ok && fclose(si->fd) != 0) { // fclose fails
    ok = FALSE;
  }

  if (ok) { // succesfull so far?
    /* Step 6:  Link the temp file to its final destination. */
    if(link(si->temp_filename, si->targ_filename) == -1) {
      LOG(LOG_ERR, "could not link %s to %s", si->temp_filename, si->targ_filename);
      ok = FALSE;
    }
    /* We've succeeded!  Now, no matter what, we return "success" */
  }

  /* Okay, delete the temporary file. If it fails, bummer. */
  unlink(si->temp_filename);

  free(si->temp_filename);
  free(si->targ_filename);
  free(si);
  return(ok);
}

// check if this is a writable directory 
static int stat_dir(char *dirname) {
  struct stat filestat;

  if(stat(dirname,&filestat) != 0) {
    return(0);
  }
  if( !S_ISDIR(filestat.st_mode) ) {
    return(0);
  }
  if((filestat.st_mode & S_IWUSR) != S_IWUSR) {
    return(0);
  }
  filestat.st_mode = 0;
  return(1);
}

// create a unique temp filename
static char *mk_tempfile(void) {
  char host[256];
  struct timeval tv;
  struct timezone tz;
  char buffer[BUFSIZ];

  gettimeofday(&tv, &tz);
  if (gethostname(host, sizeof(host)) != 0) return(NULL);
  sprintf(buffer, "%ld.%ld.%lu.%s",  
            tv.tv_sec, tv.tv_usec, (unsigned long)getpid(), host);

  return(strdup(buffer));
}

static void alarm_handler(int sig) {
  unlink(temp_filename);
}

static void set_handler(int sig, void (*h)()) {
#ifdef HASSIGACTION
  struct sigaction sa;
  sa.sa_handler = h;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(sig,&sa,(struct sigaction *) 0);
#else
  signal(sig,h);
#endif
}

