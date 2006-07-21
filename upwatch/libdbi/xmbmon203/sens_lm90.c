/*
 * Natinal Semiconductor LM75 temperature sensor chip
 *
 ***************************************************************
 * Before calling these routines, one must call method->Open() *
 * After calling these routines, one must call method->Close() *
 ***************************************************************
 *

National Semiconductor
         Chip         Temp    Volt    Fan     SMBus   IOport
        lm90           2       -       -       yes     no

Analog Devices
         Chip         Temp    Volt    Fan     SMBus   IOport
        ADM1020        2       -       -       yes     no
        ADM1021        2       -       -       yes     no
        ADM1023        2       -       -       yes     no

 *
 * by YRS
 */


#include	<stdio.h>
#include	"sensors.h"

/* external (global) data */
extern int smb_slave;
extern LM_METHODS method_smb;


#define	LM90_ADDR_START		0x90	/*0x90-0x9E*/
#define	LM90_ADDR_END		0x9E

#define LM90_LTEMP		0x00
#define LM90_RTEMPH		0x01
#define LM90_RTEMPL		0x10
#define LM90_OFFSTH		0x11
#define LM90_OFFSTL		0x12
#define LM90_VENDID		0xFE
#define LM90_DEVID		0xFF

static int lm90chipid = 0;

static	int		lm90_probe(LM_METHODS *);
static	int		lm90_ident(LM_METHODS *);
static	float	lm90_temp(LM_METHODS *, int);
static	int		lm90_fanrpm(LM_METHODS *, int);
static	float	lm90_volt(LM_METHODS *, int);

#define BUFF_LEN 128
static char buff[BUFF_LEN];

SENSOR lm90 = {
	buff,
	lm90_probe,
	lm90_temp,
	lm90_volt,
	lm90_fanrpm
};

enum lm90_chips {
	NOSENSER,
	LM90,
	ADM1020,
	ADM1023
};

static char *lm90chip[] = {
	"No Sensor",
	"Nat.Semi.Con. Chip LM90",
	"Analog Dev. Chip ADM1020",
	"Analog Dev. Chip ADM1021/1023",
	NULL };


#define LM90_chkRegNum 8

/* Register checked for probing */
static int chkReg[] = {
	0x00, 0x01, 0x02, 0x03,
	0x04, 0x05, 0x06, 0x07,
	0x08, 0x10, 0x11, 0x12,
	0x13, 0x14, 0x19, 0x20,
	0x21, 0xBF, 0xFE, 0xFF,
	-1 };


/*
 *  return 0 if not probed
 */
static	int     lm90_probe(LM_METHODS *method)
{
	int n, save;

	if (method != &method_smb)
		return 0;

	save = smb_slave;

	for (n = LM90_ADDR_START; n <= LM90_ADDR_END; ) {
		if (!(smb_slave = get_smb_slave(n, LM90_ADDR_END)))
			goto ret0;
		else {
			if (lm90_ident(method)
					&& chkReg_Probe(smb_slave, "Probing LM90 chip:\n",
							chkReg, method) >= LM90_chkRegNum)
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
	strcpy(buff, lm90chip[lm90chipid]);
	return lm90chipid;
}

static int lm90_ident(LM_METHODS *method)
{
	int vend, revd;

	vend = method->Read(LM90_VENDID);
	revd = method->Read(LM90_DEVID);
	if (vend == 0x01 && revd == 0x21)
		lm90chipid = LM90;
	else if (vend == 0x41) {
		if ((revd && 0xF0) == 0x30)
			lm90chipid = ADM1023;
		else
			lm90chipid = ADM1020;
	}

	return lm90chipid;
}

/*
 *	\retval	0xFFFF	no sensor
 *	\retval	other	temperature
 *  no = 0,1,2,...  
 */
static	float	lm90_temp( LM_METHODS *method, int no )
{
	int n = 0, k = 0;
	float offset = 0.0;

	if (no < 0 || 2 < no)
		return 0xFFFF;

	if (no == 0)
		n = method->Read(LM90_LTEMP);
	else if (no == 1) {
		if (lm90chipid != ADM1020) {
			if ((n = method->Read(LM90_OFFSTH)) == 0xFF)
				n = 0;
			if ((k = method->Read(LM90_OFFSTL)) == 0xFF)
				k = 0;
			else
				k >>= 5;
			offset = (float) n + 0.125 * (float) k;
		}
		n = method->Read(LM90_RTEMPH);
		if ((k = method->Read(LM90_RTEMPL)) == 0xFF)
			k = 0;
		else
			k >>= 5;
	}
	if (n >= 0x80)
		n = k = 0;

	return ((float) n + 0.125 * (float) k - offset);
}

/* lm90 is only for temperature sensor */

static	float	lm90_volt(LM_METHODS *method, int no)
{
	return 0xFFFF;
}

static	int		lm90_fanrpm(LM_METHODS *method, int no)
{
	return 0xFFFF;
}

