#if !defined(__smb_extemp_h__)
#define	__smb_extemp_h__

#define NUM_EXTEMP_MAX	10

enum smb_extemp_chipid {
	ex_nochip,
	ex_wl785ts,
	ex_lm90,
	ex_lm75
};

#define NO_WLCHIP
#include "sens_wl784.h"
#undef NO_WLCHIP

float smb_ExtraTemp();

#endif	/*!__smb_extemp_h__*/
