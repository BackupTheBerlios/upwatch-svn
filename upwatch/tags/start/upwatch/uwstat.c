#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <generic.h>

int uw_stat_open(char *file)
{
  int fh;

  fh = open(file, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
  return(fh);
}

int uw_stat_get(int fh, int probe)
{
  unsigned char byt;

  lseek(fh, (off_t) probe, SEEK_SET);
  if (read(fh, &byt, 1) == -1) {
    return(STAT_NONE);
  }
  return((int)byt);
}

int uw_stat_put(int fh, int probe, int color)
{
  unsigned char byt = (unsigned char) color;

  lseek(fh, (off_t) probe, SEEK_SET);
  return(write(fh, &byt, 1));
}

int uw_stat_close(int fh)
{
  return(close(fh));
}

