#include "config.h"
#include <time.h>
#include <stdio.h>
#include <glib.h>
#include "slot.h"

static int month_len_in_days(struct tm *tm)
{
  int days_in_month;

  switch (tm->tm_mon) {
    case 1: // februari
      if (tm->tm_year % 4 == 0) { // leap year?
        days_in_month = 29;
      } else {
        days_in_month = 28;
      }
      break;
    case 3:  // april
    case 5:  // june 
    case 8:  // september 
    case 10: // november
      days_in_month = 30;
      break;
    default:
      days_in_month = 31;
      break;
  }
  return(days_in_month);
}

static int year_len_in_days(struct tm *tm)
{
  int days_in_year;

  if (tm->tm_year % 4 == 0) { // leap year?
    days_in_year = 366;
  } else {
    days_in_year = 365;
  }
  return(days_in_year);
}

static int five_year_len_in_days(struct tm *tm)
{
  int five_year_days;
  int i;

  for (five_year_days=0, i=0; i < 5; i++, tm->tm_year++) {
    five_year_days += year_len_in_days(tm);
  }
  tm->tm_year -= 5;
  return(five_year_days);
}

int uw_slot(int type, gulong when, gulong *lowest, gulong *highest)
{
  struct tm *tm;
  double diff;
  int seconds_per_period = 86400;
  int seconds_per_slot;
  int slot;

  //printf(ctime(&when));

  tm = localtime((time_t *)&when);

  //printf("tm_sec = %d tm_min=%d tm_hour=%d tm_mday=%d tm_mon=%d tm_year=%d tm_wday=%d tm_wday=%d tm_yday=%d tm_isdst=%d\n", 
  //  tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday, tm->tm_yday, tm->tm_isdst);

  tm->tm_sec = 0;
  tm->tm_min = 0;
  tm->tm_hour =0;

  switch(type) {
  case SLOT_YEAR5:
    tm->tm_mday = 1;
    tm->tm_mon = 0;
    tm->tm_year -= tm->tm_year % 5;
    seconds_per_period = 86400 * five_year_len_in_days(tm);
    break;
  case SLOT_YEAR:
    tm->tm_mday = 1;
    tm->tm_mon = 0;
    seconds_per_period = 86400 * year_len_in_days(tm);
    break;
  case SLOT_MONTH:
    tm->tm_mday = 1;
    seconds_per_period = 86400 * month_len_in_days(tm);
    break;
  case SLOT_WEEK:
    tm->tm_mday -= tm->tm_wday;
    seconds_per_period = 86400*7;
    break;

  case SLOT_DAY:
    seconds_per_period = 86400;
    break;
  }
  seconds_per_slot = seconds_per_period / 200;

  *lowest = (gulong) mktime(tm);          // period start time
  diff = difftime(when, (time_t)*lowest); // # of seconds since period started
  slot = diff/seconds_per_slot;           // slotnumber within this period

  *lowest += (slot * seconds_per_slot);   // starttime of this slot.
  *highest = *lowest + seconds_per_slot;  // endtime of this slot

  //printf(ctime((time_t*)lowest));
  //printf(ctime((time_t*)highest));

  return(slot);
}

#if 0

int main(int argc, char *argv)
{
  gulong now = (gulong)time(NULL);
  gulong low, high;

  putenv("TZ=UTC");
  tzset();

  printf("now: %s", ctime(&now));

  printf("\nday slot=%d\n", uw_slot(SLOT_DAY, now, &low, &high));
  printf("low: %s", ctime(&low));
  printf("hgh: %s", ctime(&high));
  printf("\nweek slot=%d\n", uw_slot(SLOT_WEEK, now, &low, &high));
  printf("low: %s", ctime(&low));
  printf("hgh: %s", ctime(&high));
  printf("\nmonth slot=%d\n", uw_slot(SLOT_MONTH, now, &low, &high));
  printf("low: %s", ctime(&low));
  printf("hgh: %s", ctime(&high));
  printf("\nyear slot=%d\n", uw_slot(SLOT_YEAR, now, &low, &high));
  printf("low: %s", ctime(&low));
  printf("hgh: %s", ctime(&high));
  printf("\n5 year slot=%d\n", uw_slot(SLOT_5YEAR, now, &low, &high));
  printf("low: %s", ctime(&low));
  printf("hgh: %s", ctime(&high));
}


#endif
