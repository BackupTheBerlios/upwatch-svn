/*
 * Winbond and Asus hardware monitor chip
 *
 ***************************************************************
 * Before calling these routines, one must call method->Open() *
 * After calling these routines, one must call method->Close() *
 ***************************************************************
 *
 * Winbond chip: W83781D, W83782D, W83783S, W83627HF, W83697HF
 * Asus chip   : AS99127F, ASB100
 * National Semiconductor: LM78, LM78-j, LM79 [+ [2*]LM75]
 *
 * Note: LM75 is temperature only sensor chip sometimes used
 *       with LM78/79.
 *       Winbond W83781D is LM78/79 + 2 * LM75 more or less.
 *

Winbond
         Chip            Temp    Volt    Fan     SMBus   IOport
        W83781D           3       7       3       yes     yes
        W83782D           3       9       3       yes     yes
        W83783S           1-2     5-6     3       yes     no
        W83791D           3      10       5       yes     no
        W83627HF          3       9       3       yes     yes (LPC)
        W83627THF         3       7       3       yes     yes (LPC)
        W83697HF          2       6       2       no      yes (LPC)

Asus
         Chip            Temp    Volt    Fan     SMBus   IOport
        AS99127F          3       7       3       yes     no
        ASB100(Bach)      3       7       3       yes     no
        ASM58(Mozart-2)   2       4       2       yes     no
		(Mozart-2 needs a specific treatment)

National Semiconductor
         Chip            Temp    Volt    Fan     SMBus   IOport
        lm78/lm78-j       1       7       3       yes     yes
        lm79              1       7       3       yes     yes

Analog Devices
         Chip            Temp    Volt    Fan     SMBus   IOport
        adm9240           1       6       2       yes     yes

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
#include "sens_winbond.h"

/* external (global) data */
extern int pm_smb_detected;
extern int smb_slave;
extern int smb_wbtemp1, smb_wbtemp2;
extern int smb_wbtemp1_flag, smb_wbtemp2_flag;
extern LM_METHODS method_isa, method_smb;
extern int numSMBSlave, canSMBSlave[128];
extern int TyanTigerMP_flag;

#define	LM75_ADDR_START		0x90	/* 0x90-0x9E (0x48-0x4F) */
#define	LM75_ADDR_END		0x9E
#define	WINBD_ADDR_START	0x50	/* 0x50-0x5E (0x28-0x2F) */
#define	WINBD_ADDR_END		0x5E
#define	ASUSM_ADDR_FIX		0xEE	/* ASUS Mozart-2, 0xEE (0x77) only */

#define	WINBD_CONFIG	0x40
#define	WINBD_SMBADDR	0x48
#define	WINBD_DEVCID	0x49
#define	WINBD_CHIPID	0x58
#define	WINBD_VENDEX	0x4E
#define	WINBD_VENDID	0x4F
#define	ANADM_VENDID	0x3E

/* temp nr=0,1,2; volt nr=0,...6; fan nr=0,1,2 */
#define	WINBD_TEMP0		0x27
#define	ASUSB_TEMP4		0x17
#define	ASUSM_TEMP2		0x13
#define	WINBD_TEMPADDR	0x4A
#define	WINBD_VOLT(nr)	(0x20 + (nr))
#define	WINBD_FAN(nr)	(0x28 + (nr))
#define	WINBD_FANDIV	0x47
#define	WINBD_REGPIN	0x4B
#define	ASUSM_FANDIV	0xA1
#define	ANADM_TEMPCFG	0x4B

#define	WINBD_DIOSEL	0x59
#define	WINBD_VMCTRL	0x5D

static	int		winbond_probe(LM_METHODS *);
static	int		winbond_probe_act(LM_METHODS *, int);
static	float	winbond_temp(LM_METHODS *, int);
static	int		winbond_fanrpm(LM_METHODS *, int);
static	float	winbond_volt(LM_METHODS *, int);

#define BUFF_LEN 128
static char buff[BUFF_LEN];

SENSOR winbond = {
	buff,
	winbond_probe,
	winbond_temp,
	winbond_volt,
	winbond_fanrpm
};

/* chip idenfication */
static int wbdchipid = 0;
static int wbdlmid = 0;

/* temp1/2 flags address */
static int temp1_flag = 0;	/* = 0 if enabled ! */
static int temp2_flag = 0;	/* = 0 if enabled ! */
static int temp1_addr = 0;
static int temp2_addr = 0;

/* fan divisor register */
static int fan12div_reg = WINBD_FANDIV;

#define WINBD_chkRegNum 8

/* Register checked for probing */
static int chkReg[] = {
	0x40, 0x41, 0x42, 0x43,
	0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4A, 0x4B,
	0x4C, 0x4D, 0x4E, 0x4F,
	0x56, 0x58, 0x59, 0x5D,
	0x3E, 0x13, 0x17, 0xA1,
	0x20, 0x22, 0x23, 0x24,
	0x27, 0x29, 0x2A, 0x2B,
	-1 };


static void Temp_Bipolar(LM_METHODS *method)
{
	int n;

	n = method->Read(WINBD_DIOSEL) & 0x8F;
	method->Write(WINBD_DIOSEL, n);
	n = method->Read(WINBD_VMCTRL) | 0x0E;
	method->Write(WINBD_VMCTRL, n);
}

static void Init_FanDiv(LM_METHODS *method)
{
	int n;

	n = (method->Read(WINBD_FANDIV) & 0x0F) | 0xA0;
	method->Write(WINBD_FANDIV, n);
	n = (method->Read(WINBD_REGPIN) & 0x3F) | 0x80;
	method->Write(WINBD_REGPIN, n);
}

/*
 *  return 0 if not probed
 */
static	int 	winbond_probe(LM_METHODS *method)
{
	int n, save, slave = 0;

	if (method != &method_isa && method != &method_smb)
		return 0;

	save = smb_slave;

	if (method == &method_smb) {
		/* check ASUS Mozart Chip, first */
		if ((slave = get_smb_slave(ASUSM_ADDR_FIX, ASUSM_ADDR_FIX))) {
			if (winbond_probe_act(method, slave))
				goto ret1;
		}
		for (n = WINBD_ADDR_START; n <= WINBD_ADDR_END;) {
			if (!(slave = get_smb_slave(n, WINBD_ADDR_END)))
				goto ret0;
			else {
				if (winbond_probe_act(method, slave))
					goto ret1;
				else
					n = slave + 2;
			}
		}
		goto ret0;
	} else {
		if (winbond_probe_act(method, slave))
			goto ret1;
	}

ret0:
	smb_slave = save;
	return 0;
ret1:
 	/* this is TyanTigerMP specific treatment */
	if (TyanTigerMP_flag) {
		Temp_Bipolar(method);
		usleep(30000);
		Init_FanDiv(method);
	}
	if (method == &method_smb) {
		kill_smb_slave(slave);
		if(!smb_wbtemp1_flag)
			kill_smb_slave(smb_wbtemp1);
		if(!smb_wbtemp2_flag)
			kill_smb_slave(smb_wbtemp2);
	}
	return wbdchipid;
}

static	int 	winbond_probe_act(LM_METHODS *method, int slave)
{
	int i, n, nd, nc, nvl, nvu, nvx, nva;

	if (method == &method_smb)
		smb_slave = slave;
	else
		slave = 0;

	if (chkReg_Probe(slave, "Probing Winbond/Asus/LM78/79 chip:\n",
			chkReg, method) < WINBD_chkRegNum)
		goto ret0;

	nd = method->Read(WINBD_DEVCID) & 0xFE;
	nc = method->Read(WINBD_CHIPID);
	nvx = method->Read(WINBD_VENDEX);
	method->Write(WINBD_VENDEX, 0x00);
	nvl = method->Read(WINBD_VENDID);
	method->Write(WINBD_VENDEX, 0x80);
	nvu = method->Read(WINBD_VENDID);
	nva = method->Read(ANADM_VENDID);
#ifdef DEBUG
printf("DEBUG 49:0x%02X 58:0x%02X 4Fl:0x%02X 4Fu:0x%02X\n",nd,nc,nvl,nvu);
#endif

	if (nvl == 0xA3 && nvu == 0x5C) {	/* Winbond Chip */
	  switch (nc & 0xFE) {
		case 0x10:	/* 0x10 (or 0x11) */
			wbdchipid = W83781D;
			break;
		case 0x20:	/* 0x20 (or 0x21) 627HF */
		case 0x90:	/* 0x90 (or 0x91?) 627THF */
		case 0x1A:	/* 0x1A (??)  627THF-A */
			wbdchipid = W83627HF;
			break;
		case 0x30:	/* 0x30 (or 0x31) */
			wbdchipid = W83782D;
			if (nc == 0x31)
				wbdchipid = AS99127F;	/* very special, but ... */
			break;
		case 0x40:	/* 0x40 (or 0x41) */
			wbdchipid = W83783S;
			break;
		case 0x60:	/* 0x60 (or 0x61) */
			wbdchipid = W83697HF;
			break;
		case 0x70:	/* 0x70 (or 0x71) */
			wbdchipid = W83791D;
			break;
		default:
#ifdef ALLOW_UNKNOWN
			wbdchipid = WBUNKNOWN;
#else
			goto ret0;
#endif
	  }
	} else if ((nvl == 0xC3 && nvu == 0x12) && nc == 0x31) {	/* Asus Chip */
			wbdchipid = AS99127F;
	} else if ((nvl == 0x94 && nvu == 0x06) && nc == 0x31) {	/* Asus Chip */
			wbdchipid = ASB100;
	} else if ( smb_slave == ASUSM_ADDR_FIX &&		/* Mozart-2, special */
				((nvx == 0x94 && nvl == 0x36 && nc == 0x56) ||
				 (nvx == 0x94 && nvl == 0x06 && nc == 0x56) ||
				 (nvx == 0x5C && nvl == 0xA3 && nc == 0x10))) {
			wbdchipid = ASM58;
	} else if (nd == 0x20 || nd == 0x40) { /* 0x20, 0x40 */
			wbdchipid = LM78;
	} else if (nd == 0xC0) { /* 0xC0 */
			wbdchipid = LM79;
	} else if (nva == 0x23) { /* ADM9240 */
			wbdchipid = ADM9240;
	} else
#ifdef ALLOW_UNKNOWN
			wbdchipid = UNKNOWN;
#else
			goto ret0;
#endif

	strcpy(buff, winbchip[wbdchipid]);

	wbdlmid = wbdchipid;
	if (wbdchipid == WBUNKNOWN || wbdchipid >= LM78)
		wbdlmid = W83781D;

	if (wbdchipid == ASB100)	/* Asus Bach */
		wbdlmid = W83781D;

	if (wbdchipid == ASM58) {	/* Asus Mozart-2 */
		wbdlmid = W83781D;
		temp1_flag = temp2_flag = 1;	/* disable! */
		fan12div_reg = ASUSM_FANDIV;
		method->Write(WINBD_CONFIG, 0x01);	/* init. chip */
		goto ret1;
	}

	if (wbdchipid == ADM9240) {
		temp1_flag = temp2_flag = 1;	/* disable! */
		method->Write(WINBD_CONFIG, 0x01);	/* init. chip */
		goto ret1;
	}

	if (method == &method_isa && wbdchipid >= LM78) {
		temp1_flag = temp2_flag = 1;	/* disable! */
		goto ret1;
	}

/* Checking Extra temperatures Temp1, Temp2 */

	if (wbdchipid >= LM78) { /* possibility of LM75 sensor */
		i = set_smb_Extemp(LM75_ADDR_START, LM75_ADDR_END,
				&smb_wbtemp2, &smb_wbtemp1);
		temp2_flag = i >> 1;
		temp1_flag = i & 0x01;
		info_Extemp(method, temp1_flag, temp2_flag);
		if (!temp1_flag || !temp2_flag)
			strcat(winbond.Name, "+LM75");
		goto ret1;
	}

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
	if (method == &method_smb) {
		smb_wbtemp1_flag = temp1_flag;
		smb_wbtemp2_flag = temp2_flag;
	}
	return wbdchipid;
ret0:
	return 0;
}


/*
 *	\retval	0xFFFF	no sensor
 *	\retval	other	temperature
 *  no = 0,1,2
 */
static	float	winbond_temp(LM_METHODS *method, int no)
{
	int n = 0;
	float f;

	if (no < 0 || 2 < no)
		return 0xFFFF;
	if (no == 2 &&
		(wbdchipid == W83783S || wbdchipid == W83697HF || wbdchipid == ASM58))
		return 0xFFFF;

	if (no == 0) {
		f = (float) method->Read(WINBD_TEMP0);
		if (wbdchipid == ADM9240) {
			n = method->Read(ANADM_TEMPCFG);
			if (n & 0x80)
				f += 0.5;
		}
		return f;
	} else if (no == 1) {
		if (wbdchipid == ASB100)
			return (float) method->Read(ASUSB_TEMP4);
		if (wbdchipid == ASM58)
			return (float) method->Read(ASUSM_TEMP2);
		else if (!temp1_flag)
			n = method->ReadTemp1();
#ifdef SYRS
	} else if (no == 2 && !temp2_flag) {
#else
	} else if (no == 2) {
		if (wbdchipid == ASB100) {
		  if (!temp1_flag)
			n = method->ReadTemp1();
		} else if (!temp2_flag)
#endif
			n = method->ReadTemp2();
	}

	if ((n & 0xFF) >= 0x80)
		n = 0;

	f = (float) (n & 0xFF) + 0.5 * ((n & 0xFF00) >> 15);

	if (wbdchipid == AS99127F && pm_smb_detected == ICH801SMB) {
	/* very special treatment for AS99127F with ICH chipsets */
		if (no == 1 && (-32.0 < f && f <= 105.0)) {
			f *= 0.697;
			f += 25.0;
		}
	}
	return f;
}


/*
 *	\retval	0x0000FFFF	no sensor
 *  no = 0,1,2,...,6
 */
static	float	winbond_volt(LM_METHODS *method, int no)
{
	int n;
	float f = 0.0;

	if (no < 0 || 6 < no)
		return 0xFFFF;
	if (wbdchipid == ASM58 && (no == 1 || no > 4))
		return 0xFFFF;
	if (wbdchipid == ADM9240 && (no > 5))
		return 0xFFFF;

	n = method->Read(WINBD_VOLT(no));
	switch (no) {
		case 0:
		case 1:
		case 2:
			f = n * 0.016;
			break;
		case 3:
			f = n * 0.016 * 1.68;
			break;
		case 4:
			f = n * 0.016 * 3.800;
			break;
		case 5:
			if (wbdlmid == AS99127F)
				f = - n * 0.016 * 3.968;
			else if (wbdlmid == W83781D)
				f = - n * 0.016 * 3.477;
			else
				f = ( n * 0.016  - 3.6 * 0.8056) / 0.1944;
			break;
		case 6:
			if (wbdlmid == W83781D || wbdlmid == AS99127F)
				f = - n * 0.016 * 1.500;
			else
				f = ( n * 0.016  - 3.6 * 0.6818) / 0.3182;
	}

	return f;
}


/*
	Controlling Fan Divisor for 1st/2nd fans: CR = 0x47.
	1st two bits for fan1, 2nd two bits for fan2

         7     4 3     0
        +-+-+-+-+-+-+-+-+     xx = 00,01,10,11  div1fac = 1,2,4,8
        |y y|x x| VolID |     yy = 00,01,10,11  div2fac = 1,2,4,8
        +-+-+-+-+-+-+-+-+    initial values: xx=01, yy=01

	Controlling Fan Divisor for 3rd fan: CR = 0x4B.
	1st two bits for fan3

         7 6 5         0
        +-+-+-+-+-+-+-+-+
        |z z|           |     zz = 00,01,10,11  div3fac = 1,2,4,8
        +-+-+-+-+-+-+-+-+    initial values: zz=01

	3rd fan divisor available for Winbond (not for LM78/79).

    Bit 2 of Fan Divisor: CR = 0x5D (Winbond chips except 781D).

         7 6 5         0
        +-+-+-+-+-+-+-+-+
        |z|y|x|         |     x, y, z for bit 2 of fan 1, 2, 3
        +-+-+-+-+-+-+-+-+
 */

/*
 *	\retval	0xFFFF no sensor
 *  no = 0,1,2
 *
 *  Clock is 22.5kHz (22,500 x 60 = 1350000 counts/minute)
 */
static	int		winbond_fanrpm(LM_METHODS *method, int no)
{
	int r, n1 = 0x50, n2 = 0x40, n3 = 0x00;
	static int div[3] = {1,1,1};

	if (no < 0 || 2 < no)
		return 0xFFFF;
	if (no == 2
		&& (wbdchipid == W83697HF || wbdchipid == ASM58
			|| wbdchipid == ADM9240))
		return 0xFFFF;

	if (W83782D <= wbdchipid && wbdchipid <= W83697HF)
		n3 = method->Read(WINBD_VMCTRL);	/* bit 2 */
	if (no != 2) {
		n1 = method->Read(fan12div_reg);	/* bit 0,1 */
		div[0] = ((n1 >> 4) & 0x03) | ((n3 & 0x20) >> 3);
		div[1] =  (n1 >> 6) | ((n3 & 0x40) >> 4);
	} else if (wbdchipid < LM78) {
		n2 = method->Read(WINBD_REGPIN);	/* bit 0,1 */
		div[2] =  (n2 >> 6) | ((n3 & 0x80) >> 5);
	}

	r = method->Read(WINBD_FAN(no));
	if (r == 0xFF) {
		/* change divisor for the sake of next call ! */
		if (no != 2) {
			if (div[no] < 3)
				++(div[no]);
			else
				div[no] = 0;
			r = (n1 & 0x0F) | ((div[0] & 0x03) << 4) | ((div[1] & 0x03) << 6);
			method->Write(fan12div_reg, r);
		} else if (wbdchipid < LM78) {
			if (div[no] < 3)
				++(div[no]);
			else
				div[no] = 0;
			r = (n2 & 0x3F) | ((div[2] & 0x03) << 6);
			method->Write(WINBD_REGPIN, r);
		}
		if (W83782D <= wbdchipid && wbdchipid <= W83697HF) {
			r = (n3 & 0x1F) | ((div[0] & 0x04) << 3) |
				((div[1] & 0x04) << 4) | ((div[2] & 0x04) << 5);
			method->Write(WINBD_VMCTRL, r);
		}
		return 0xFFFF;
	} else if (r == 0) {
		return 0xFFFF;
	}

	return 1350000 / (r * (1 << div[no]));
}
