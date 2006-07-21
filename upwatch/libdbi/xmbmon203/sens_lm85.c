/*
 * National Semiconductor LM85 and compatible hardware monitor chips
 *
 ***************************************************************
 * Before calling these routines, one must call method->Open() *
 * After calling these routines, one must call method->Close() *
 ***************************************************************
 *

National Semiconductor
         Chip         Temp    Volt    Fan     SMBus   IOport
        lm85           3       5       4       yes     no

Analog Devices
         Chip         Temp    Volt    Fan     SMBus   IOport
        adm1024        2       5       2       yes     no
        adm1025        2       5       -       yes     no
        adm1027        3       5       4       yes     no
        adt7463        3       5       4       yes     no

Standard Microsystem
         Chip         Temp    Volt    Fan     SMBus   IOport
        emc6d10x       3       5       4       yes     no

 *
 * by YRS
 */


#include	<stdio.h>
#include	<string.h>
#include	"sensors.h"

/* external (global) data */
extern int smb_slave;
extern LM_METHODS method_isa, method_smb;
extern int numSMBSlave, canSMBSlave[128];


#define	LM85_ADDR_START		0x58	/*0x58-0x5A*/
#define	LM85_ADDR_END		0x5C

#define	LM85_DEVID		0x3D
#define	LM85_VENDID		0x3E
#define	LM85_VERSTEP	0x3F
#define	LM85_CONF		0x40

#define	LM85_TEMP(nr)	(0x25 + (nr))
#define	LM85_VOLT(nr)	(0x20 + (nr))
#define	LM85_FANLSB(nr)	(0x28 + (nr) * 2)
#define	LM85_FANMSB(nr)	(0x29 + (nr) * 2)
#define	LM85_VID		0x43
#define ADM_24FAN(nr)	(0x28 + (nr))
#define ADM_24FANDIV	0x47
#define ADM_24MODE		0x16
#define ADM_TEMPOFF(nr)	(0x70 + (nr))
#define ADM_EXTRES1		0x76
#define ADM_EXTRES2		0x77
#define ADM_FANPPR		0x7B

static int lm85chipid = 0;

static float Vfac;

static	int		lm85_probe(LM_METHODS *);
static	int		lm85_chk(LM_METHODS *);
static	float	lm85_temp(LM_METHODS *, int);
static	int		lm85_fanrpm(LM_METHODS *, int);
static	float	lm85_volt(LM_METHODS *, int);
#if 0
static	int		lm85_fan_ppr(LM_METHODS *, int);
static	void	lm85_fan_ppr_set(LM_METHODS *, int, int);
#endif

#define BUFF_LEN 128
static char buff[BUFF_LEN];

static int adm_24fan[2] = {0, 0};

SENSOR lm85 = {
	buff,
	lm85_probe,
	lm85_temp,
	lm85_volt,
	lm85_fanrpm
};


enum lm85_chips {
	NOSENSER,
	LM85,
	EMC6D10X,
	ADM1024,
	ADM1025,
	ADM1027,
	ADT7463,
};

static char *lm85chip[] = {
	"No Sensor",
	"Nat.Semi.Con. Chip LM85",
	"SMSC Chip EMC6D10X",
	"Analog Dev. Chip ADM1024",
	"Analog Dev. Chip ADM1025",
	"Analog Dev. Chip ADM1027",
	"Analog Dev. Chip ADT7463",
	NULL };


#define LM85_chkRegNum 20

/* Register checked for probing */
static int chkReg[] = {
	0x20, 0x21, 0x22, 0x23,
	0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2A, 0x2B,
	0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33,
	0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B,
	0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43,
	0x70, 0x71, 0x72, 0x73,
	0x76, 0x77, 0x78, 0x7B,
	-1 };


/*
 *  return 0 if not probed
 */
static	int     lm85_probe(LM_METHODS *method)
{
	int n, save;

	if (method != &method_smb)
		return 0;

	save = smb_slave;

	for (n = LM85_ADDR_START; n <= LM85_ADDR_END;) {
		if (!(smb_slave = get_smb_slave(n, LM85_ADDR_END)))
			goto ret0;
		else {
			if (lm85_chk(method)
				&& chkReg_Probe(smb_slave,
					"Probing LM85/compatible chip:\n", chkReg, method)
						>= LM85_chkRegNum)
				goto ret1;
			else
				n = smb_slave + 2;
		}
	}

ret0:
	smb_slave = save;
	return 0;
ret1:
	kill_smb_slave(smb_slave);
	if (lm85chipid == ADM1024) {
		n = method->Read(ADM_24MODE);
		adm_24fan[0] = n & 0x01;	/* fan1 present if n & 0x01 = 0 */
		adm_24fan[1] = n & 0x02;	/* fan2 present if n & 0x02 = 0 */
	}
	if (lm85chipid < ADM1027)
		Vfac = 1. / (float) 0xFF;
	else
		Vfac = 1. / (float) 0x3FF;
	return lm85chipid;
}

static	int		lm85_chk(LM_METHODS *method)
{
	int vendor, verstep;

	vendor  = method->Read(LM85_VENDID);
	verstep = method->Read(LM85_VERSTEP);

	if (vendor == 0x01) {	/* National Semicon. */
		if ((verstep & 0xf0) == 0x60)
			lm85chipid = LM85;
		else
			goto ret0;
	} else if (vendor == 0x41) {	/* Analog Devices */
		if ((verstep & 0xF0) == 0x10)
			lm85chipid = ADM1024;
		else if ((verstep & 0xF0) == 0x20)
			lm85chipid = ADM1025;
		else if (verstep == 0x60)
			lm85chipid = ADM1027;
		else if (verstep == 0x62)
			lm85chipid = ADT7463;
		else
			goto ret0;
	} else if (vendor == 0x5C) {	/* Standard Microsystem Corp. */
		if ((verstep & 0xF0) == 0x60)
			lm85chipid = EMC6D10X;
		else
			goto ret0;
	} else
			goto ret0;
		
	strcpy(buff, lm85chip[lm85chipid]);
	return lm85chipid;
ret0:
	return 0;
}

/*
 *	\retval	0xFFFF	no sensor
 *	\retval	other	temperature
 *  no = 0,1,...  
 */
static	float	lm85_temp( LM_METHODS *method, int no )
{
	int n, ne;
	float ext = 0.0, offset = 0.0;

	if (no < 0 || 2 < no)
		return 0xFFFF;

	if (lm85chipid == ADM1024 || lm85chipid == ADM1025) {
		if (no == 2)
			return 0xFFFF;
		else
			no++;
	} else if (lm85chipid >= ADM1027) {
		ne = method->Read(ADM_EXTRES2);	
		ext = 0.25 * ((ne >> ((no + 1) * 2)) & 0x03);
		n = method->Read(ADM_TEMPOFF(no));
		if (n > 0x80)
			n -= 0xFF;
		offset = (float) n;
	}
	n = method->Read(LM85_TEMP(no));
	if (n == 0x80)
		return 0xFFFF;
	else if (n > 0x80)
		return (float) (n - 0xFF) + ext - offset;
	else
		return (float) n + ext - offset;
}


/*
 *	\retval	0x0000FFFF	no sensor
 *  no = 0,1,2,...  
 */

static float Vtab[5] = {
	2.5  * 4./3.,	/* 2.5V */
	2.25 * 4./3.,	/* Vcore (2.25) */
	3.3  * 4./3.,	/* 3.3V */
	5.0  * 4./3.,	/* 5.0V */
	12.0 * 4./3.	/* 12.0V */
};

static	float	lm85_volt(LM_METHODS *method, int no)
{
	int n, ne = 0;

	if (no < 0 || 4 < no)
		return 0xFFFF;

	if (lm85chipid >= ADM1027) {
		if (no == 4)
			ne = method->Read(ADM_EXTRES2) & 0x03;
		else
			ne = (method->Read(ADM_EXTRES1) >> (no * 2)) & 0x03;
	}
	n = method->Read(LM85_VOLT(no));
	if (lm85chipid >= ADM1027)
		n = (n << 2) | ne;

	return Vtab[no] * Vfac * (float) n;
}


/*
 *	\retval	0x0000FFFF	no sensor
 *  no = 0,1,...  
 */
static	int		lm85_fanrpm(LM_METHODS *method, int no)
{
	int lsb, msb, r, n;
	int p = 2;	/* default number of pulse per revolution */
	static int ppr[4] = {2,2,2,2}, div[2] = {1,1};

	if (no < 0 || 3 < no || lm85chipid == ADM1025)
		return 0xFFFF;

	if (lm85chipid == ADM1024) {
		if (no > 1 || adm_24fan[no])
			return 0xFFFF;
		n = method->Read(ADM_24FANDIV);
		div[0] = (n >> 4) & 0x03;
		div[1] =  n >> 6;
		r = method->Read(ADM_24FAN(no));
		if (r == 0)
			return 0xFFFF;
		else if (r == 0xFF) {
			/* change divisor for the sake of next call ! */
			if (div[no] < 3)
				++(div[no]);
			else
				div[no] = 0;
			r = (n & 0x0F) | (div[0] << 4) | (div[1] << 6);
			method->Write(ADM_24FANDIV, r);
			return 0xFFFF;
		} else
			return 1350000 / (r * (1 << div[no]));
	}

	if (lm85chipid >= ADM1027)
		p = ((method->Read(ADM_FANPPR) >> (no * 2)) & 0x03) + 1;
	ppr[no] = p;

	lsb = method->Read(LM85_FANLSB(no));
	msb = method->Read(LM85_FANMSB(no));
	if ((lsb == 0xFF && msb == 0xFF) || (lsb == 0 && msb == 0))
		return 0xFFFF;
	else
		return 5400000 / (p * ((msb << 8) | lsb));
}

#if 0

static int lm85_fan_ppr(LM_METHODS *method, int no)
{
	int p = 2;

	if (lm85chipid >= ADM1027)
		p = ((method->Read(ADM_FANPPR) >> (no * 2)) & 0x03) + 1;

	return p;
}

static void lm85_fan_ppr_set(LM_METHODS *method, int no, int p)
{
	int n;

	if (lm85chipid >= ADM1027) {
		n = method->Read(ADM_FANPPR);
		n &= ((p - 1) & 0x03) << (no * 2);
		method->Write(ADM_FANPPR, n);
	}
}

#endif
