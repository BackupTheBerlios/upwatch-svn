/*
 * Winbond new chips
 *
 ***************************************************************
 * Before calling these routines, one must call method->Open() *
 * After calling these routines, one must call method->Close() *
 ***************************************************************
 *
 * Winbond chip: W83L784R, W83L785R, W83L785TS-S
 *

Winbond
         Chip            Temp    Volt    Fan     SMBus   IOport
        W83L784R          3       4       2       yes     no
        W83L785R          2       4       2       yes     no
        W83L785TS-S       1       0       0       yes     no

 *
 * by YRS
 */

/* To allow "unknown" (fuzzy probing), define this */
/*	#define ALLOW_UNKNOWN	*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pci_pm.h"
#include "sensors.h"
#include "sens_wl784.h"

/* external (global) data */
extern int pm_smb_detected;
extern int smb_slave;
extern int smb_wbtemp1, smb_wbtemp2;
extern int smb_wbtemp1_flag, smb_wbtemp2_flag;
extern LM_METHODS method_isa, method_smb;
extern int numSMBSlave, canSMBSlave[128];

#define	WINBD_ADDR_START	0x50	/* 0x50-0x5E (0x28-0x2F) */
#define	WINBD_ADDR_END		0x5E

#define	WINBD_CONFIG	0x40
#define	WINBD_SMBADDR	0x4A
#define	WINBD_VENDIDL	0x4C
#define	WINBD_VENDIDH	0x4D
#define	WINBD_CHIPID	0x4E

/* temp nr=0,1,2; volt nr=0,...3; fan nr=0,1 */
#define	WINBD_TEMP0		0x27
#define	WINBD_TEMP1		0x26
#define	WINBD_TEMPADDR	0x4B
#define	WINBD_VOLT(nr)	(0x20 + (nr))
#define	WINBD_FAN(nr)	(0x28 + (nr))
#define	WINBD_FANDIV	0x49

#define WINBD_DIOSEL	0x53

static	int		wl784_probe(LM_METHODS *);
static	int		wl784_probe_act(LM_METHODS *, int);
static	float	wl784_temp(LM_METHODS *, int);
static	int		wl784_fanrpm(LM_METHODS *, int);
static	float	wl784_volt(LM_METHODS *, int);

#define BUFF_LEN 128
static char buff[BUFF_LEN];

SENSOR wl784 = {
	buff,
	wl784_probe,
	wl784_temp,
	wl784_volt,
	wl784_fanrpm
};

/* chip idenfication */
static int wbdchipid = 0;

/* temp1/2 flags address */
static int temp1_flag = 0;	/* = 0 if enabled ! */
static int temp2_flag = 0;	/* = 0 if enabled ! */
static int temp1_addr = 0;
static int temp2_addr = 0;

#define WINBD_chkRegNum 8

/* Register checked for probing */
static int chkReg[] = {
	0x40, 0x41, 0x42, 0x43,
	0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4A, 0x4B,
	0x4C, 0x4D, 0x4E, 0x4F,
	0x20, 0x21, 0x22, 0x23,
	0x26, 0x27, 0x28, 0x29,
	0x2B, 0x2C, 0x2D, 0x2E,
	-1 };


/*
 *  return 0 if not probed
 */
static	int 	wl784_probe(LM_METHODS *method)
{
	int n, save, slave;

	if (method != &method_smb)
		return 0;

	save = smb_slave;

	for (n = WINBD_ADDR_START; n <= WINBD_ADDR_END;) {
		if (!(slave = get_smb_slave(n, WINBD_ADDR_END)))
			goto ret0;
		else {
			if (wl784_probe_act(method, slave))
				goto ret1;
			else
				n = slave + 2;
		}
	}

ret0:
	smb_slave = save;
	return 0;
ret1:
	kill_smb_slave(slave);
	if(!smb_wbtemp1_flag)
		kill_smb_slave(smb_wbtemp1);
	if(!smb_wbtemp2_flag)
		kill_smb_slave(smb_wbtemp2);
	return wbdchipid;
}

static	int 	wl784_probe_act(LM_METHODS *method, int slave)
{
	int n, nc, nvl, nvh;

	smb_slave = slave;

	if (chkReg_Probe(slave, "Probing Winbond W83L78x chip:\n",
			chkReg, method) < WINBD_chkRegNum)
		goto ret0;


	nvl = method->Read(WINBD_VENDIDL);
	nvh = method->Read(WINBD_VENDIDH);
	nc = method->Read(WINBD_CHIPID);
#ifdef DEBUG
printf("DEBUG 4C:0x%02X 4D:0x%02X 4E:0x%02X\n",nvl,nvh,nc);
#endif

	if (nvl == 0xA3 && nvh == 0x5C) {	/* Winbond Chip */
		switch (nc & 0xFE) {
			case 0x50:	/* 0x50 (or 0x51) */
				wbdchipid = W83L784R;
				break;
			case 0x60:	/* 0x60 (or 0x61) */
				wbdchipid = W83L785R;
				break;
			case 0x70:	/* 0x70 (or 0x71) */
				wbdchipid = W83L785TS;
				break;
			default:
				goto ret0;
		}
	} else
		goto ret0;

	strcpy(wl784.Name, wlchip[wbdchipid]);

/* Checking Extra temperatures Temp1, Temp2 */

	if (wbdchipid != W83L784R)
		goto ret1;

	n = method->Read(WINBD_TEMPADDR);
	if (!(temp1_flag = (n & 0x08) >> 3)) {
		temp1_addr = smb_wbtemp1;
		smb_wbtemp1 = 2 * ( 0x48 + (n & 0x07) );
		if (method->ReadTemp1() == 0xFFFF) {
			temp1_flag = 1;	/* disable! */
			smb_wbtemp1 = temp1_addr;
		}
	}

	if (!(temp2_flag = (n & 0x80) >> 7)) {
		temp2_addr = smb_wbtemp2;
		smb_wbtemp2 = 2 * ( 0x48 + ((n & 0x70) >> 4) );
		if (method->ReadTemp2() == 0xFFFF) {
			temp2_flag = 1;	/* disable! */
			smb_wbtemp2 = temp2_addr;
		}
	}
	info_Extemp(method, temp1_flag, temp2_flag);

ret1:
	smb_wbtemp1_flag = temp1_flag;
	smb_wbtemp2_flag = temp2_flag;
	return wbdchipid;
ret0:
	return 0;
}


/*
 *	\retval	0xFFFF	no sensor
 *	\retval	other	temperature
 *  no = 0,1,2
 */
static	float	wl784_temp(LM_METHODS *method, int no)
{
	int n = 0;
	float f;

	if (no < 0 || 2 < no)
		return 0xFFFF;
	if (no == 2 && wbdchipid == W83L785R)
		return 0xFFFF;
	if (no >= 1 && wbdchipid == W83L785TS)
		return 0xFFFF;

	if (no == 0)
		return (float) method->Read(WINBD_TEMP0);
	else if (no == 1) {
		if (wbdchipid == W83L785R)
			return (float) method->Read(WINBD_TEMP1);
		else
			n = method->ReadTemp1();
	} else if (no == 2) {
			n = method->ReadTemp2();
	}

	if ((n & 0xFF) >= 0x80)
		n = 0;

	f = (float) (n & 0xFF) + 0.5 * ((n & 0xFF00) >> 15);

	return f;
}


/*
 *	\retval	0x0000FFFF	no sensor
 *  no = 0,1,2,...,6
 */
static	float	wl784_volt(LM_METHODS *method, int no)
{
	int n;
	float f = 0.0;

	if (wbdchipid == W83L785TS)
		return 0xFFFF;
	if (no < 0 || 3 < no)
		return 0xFFFF;

	n = method->Read(WINBD_VOLT(no));
	switch (no) {
		case 0:
			f = n * 0.016;
		case 1:
			if (wbdchipid == W83L784R)
				f = n * 0.016 * 3.3434;
			else
				f = n * 0.016 * 2;
		case 2:
			f = n * 0.016;
			break;
		case 3:
			if (wbdchipid == W83L784R)
				f = n * 0.016 * 1.68;
			else
				f = n * 0.016 * 3;
			break;
	}

	return f;
}


/*
	Controlling Fan Divisor: CR = 0x49.
	lowest 3bits for fan1, 4-6th bits for fan2.

         7       3     0
        +-+-+-+-+-+-+-+-+    xxx = 000,001,010,011,...  div1fac = 1,2,4,8,...
        |  y y y|  x x x|    yyy = 000,001,010,011,...  div2fac = 1,2,4,8,...
        +-+-+-+-+-+-+-+-+    initial values: xx=001, yy=001

 */

/*
 *	\retval	0xFFFF no sensor
 *  no = 0,1
 *
 *  Clock is 22.5kHz (22,500 x 60 = 1350000 counts/minute)
 */
static	int		wl784_fanrpm(LM_METHODS *method, int no)
{
	int r, n1 = 0x11;
	static int div[2] = {1,1};

	if (wbdchipid == W83L785TS)
		return 0xFFFF;
	if (no < 0 || 1 < no)
		return 0xFFFF;

	n1 = method->Read(WINBD_FANDIV);
	div[0] =  n1 & 0x07 ;
	div[1] = (n1 >> 4) & 0x07;

	r = method->Read(WINBD_FAN(no));
	if (r == 0xFF) {
		/* change divisor for the sake of next call ! */
		if (div[no] < 7)
			++(div[no]);
		else
			div[no] = 0;
		r = (n1 & 0x88) | div[0] | (div[1] << 4);
		method->Write(WINBD_FANDIV, r);
		return 0xFFFF;
	} else if (r == 0) {
		return 0xFFFF;
	}

	return 1350000 / (r * (1 << div[no]));
}
