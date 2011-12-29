#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <slot_options.h>
#include "db.h"
#include "generic.h"
#include "slot.h"

int main (int argc, char *argv[])
{
  int arg_ct, slot;
  time_t arg;
  gulong lowest, highest;
  char start[40], end[40];

#ifdef ERRSKIP_OPTERR
  ERRSKIP_OPTERR;  // Don't barf on unknown commandline or configfile options. Only available in newer autogen versions
#endif

  arg_ct = optionProcess( &progOptions, argc, argv );
  argc -= arg_ct;
  argv += arg_ct;

  if (argc < 1) {
    fprintf(stderr, "Usage: slot arg\n");
    exit (1);
  }
  arg = atoi(*argv);
  printf("%s", ctime(&arg));

  slot = uw_slot(SLOT_DAY, arg, &lowest, &highest);
  strcpy(start, ctime(&lowest)); start[strlen(start)-1] = 0;
  strcpy(end, ctime(&highest)); end[strlen(end)-1] = 0;
  printf("%6s-slot %3d:  %lu %s => %lu %s\n", "day", slot, lowest, start, highest, end); 

  slot = uw_slot(SLOT_WEEK, arg, &lowest, &highest);
  strcpy(start, ctime(&lowest)); start[strlen(start)-1] = 0;
  strcpy(end, ctime(&highest)); end[strlen(end)-1] = 0;
  printf("%6s-slot %3d:  %lu %s => %lu %s\n", "week", slot, lowest,start, highest, end); 

  slot = uw_slot(SLOT_MONTH, arg, &lowest, &highest);
  strcpy(start, ctime(&lowest)); start[strlen(start)-1] = 0;
  strcpy(end, ctime(&highest)); end[strlen(end)-1] = 0;
  printf("%6s-slot %3d:  %lu %s => %lu %s\n", "month", slot, lowest, start, highest, end); 

  slot = uw_slot(SLOT_YEAR, arg, &lowest, &highest);
  strcpy(start, ctime(&lowest)); start[strlen(start)-1] = 0;
  strcpy(end, ctime(&highest)); end[strlen(end)-1] = 0;
  printf("%6s-slot %3d:  %lu %s => %lu %s\n", "year", slot, lowest, start, highest, end); 

  slot = uw_slot(SLOT_YEAR5, arg, &lowest, &highest);
  strcpy(start, ctime(&lowest)); start[strlen(start)-1] = 0;
  strcpy(end, ctime(&highest)); end[strlen(end)-1] = 0;
  printf("%6s-slot %3d:  %lu %s => %lu %s\n", "5year", slot, lowest, start, highest, end); 

  return 0;
}

