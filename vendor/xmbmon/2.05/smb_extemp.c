/*
	Getting temperature data from extra sensors
	Here, only the sensors connected to SMBus are used

	by YRS.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "smb_extemp.h"
#include "smbuses.h"

/* smb_extemp, global */
int num_extemp_chip = 0;
int smb_extemp_chip[NUM_EXTEMP_MAX];
int smb_extemp_slave[NUM_EXTEMP_MAX];

/* smbus base address, global */
extern int smb_base;

#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
extern int smbioctl_readB(int, int, int);
extern int smbioctl_readW(int, int, int);
#else			/* using own SMBus IO routines */
/* smbus io routine, global */
extern SMBUS_IO *smbus;
#endif

#define LM75_TEMP		0x00

#define LM90_LTEMP		0x00
#define LM90_RTEMPH		0x01
#define LM90_RTEMPL		0x10
#define LM90_OFFSTH		0x11
#define LM90_OFFSTL		0x12

#define	WINBD_TEMP0		0x27
#define	WINBD_TEMP1		0x26


float	smb_ExtraTemp()
{
	int slave, chipid, chip;
	int n = 0, k = 0;
	float f = 0.0, offset = 0.0;
	int (*readB)(int, int, int);
	int (*readW)(int, int, int);

#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
	readB = smbioctl_readB;
	readW = smbioctl_readW;
#else
	readB = smbus->ReadB;
	readW = smbus->ReadW;
#endif

	chip = num_extemp_chip - 1;		/* using the last chip */
	if (chip < 0)
		return 0xFFFF;

	chipid = smb_extemp_chip[chip];
	slave = smb_extemp_slave[chip];
	if (chipid == ex_wl785ts) {
		n = readB(smb_base, slave, WINBD_TEMP0);
		f = (float) n;
	} else if (chipid == ex_lm90) {
		if ((n = readB(smb_base, slave, LM90_OFFSTH)) == 0xFF)
			n = 0;
		if ((k = readB(smb_base, slave, LM90_OFFSTL)) == 0xFF)
			k = 0;
		else
			k >>= 5;
		offset = (float) n + 0.125 * (float) k;
		n = readB(smb_base, slave, LM90_RTEMPH);
		if ((k = readB(smb_base, slave, LM90_RTEMPL)) == 0xFF)
			k = 0;
		else
			k >>= 5;
		if (n >= 0x80)
			n = k = 0;
		f = (float) n + 0.125 * (float) k;
	} else if (chipid == ex_lm75) {
		n = readW(smb_base, slave, LM75_TEMP);
		if ((n & 0xFF) >= 0x80)
			n = 0;
		f = (float) (n & 0xFF) + 0.5 * ((n & 0xFF00) >> 15);
	}
	return f;
}
