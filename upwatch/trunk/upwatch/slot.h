#include <time.h>

#define SLOT_DAY 1
#define SLOT_WEEK 2
#define SLOT_MONTH 3
#define SLOT_YEAR 4
#define SLOT_YEAR5 5

//extern int uw_slot(int type, time_t when);

int uw_slot(int type, gulong when, gulong *lowest, gulong *highest);

