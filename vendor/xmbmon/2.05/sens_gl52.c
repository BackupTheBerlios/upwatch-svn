/*
 * Genesys Logic GL518SM/GL520SM hardware monitor chip
 *
 ***************************************************************
 * Before calling these routines, one must call method->Open() *
 * After calling these routines, one must call method->Close() *
 ***************************************************************
 *

Genesys Logic
         Chip         Temp    Volt    Fan     SMBus   IOport
        gl518sm        1       4       2       yes     no
        gl520sm        2(1)    4(5)    2       yes     no

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


#define	GL52_ADDR_START		0x58	/*0x58-0x5A*/
#define	GL52_ADDR_END		0x5A

#define	GL52_CHIPID		0x00
#define	GL52_REVNUM		0x01
#define	GL52_CONF		0x03

#define	GL52_TEMP1		0x04
#define	GL52_TEMP2		0x0E
#define	GL52_VIN0		0x15
#define	GL52_VIN1		0x14
#define	GL52_VIN2		0x13
#define	GL52_VIN3		0x0D
#define	GL52_VIN4		0x0E
#define	GL52_FANW		0x07
#define	GL52_FANDIV		0x0F

static int gl52chipid = 0;
static int gl52_mode = 1;
static int temp_offset = 130;

static	int		gl52_probe(LM_METHODS *);
static	float	gl52_temp(LM_METHODS *, int);
static	int		gl52_fanrpm(LM_METHODS *, int);
static	float	gl52_volt(LM_METHODS *, int);

#define BUFF_LEN 128
static char buff[BUFF_LEN];

SENSOR gl52 = {
	buff,
	gl52_probe,
	gl52_temp,
	gl52_volt,
	gl52_fanrpm
};


enum gl52_chips {
	NOSENSER,
	GL518SM_r00,
	GL518SM_r80,
	GL520SM
};

static char *gl52chip[] = {
	"No Sensor",
	"GL518SM_rev00",
	"GL518SM_rev80",
	"GL520SM",
	NULL };


#define GL52_chkRegNum 10

/* Register checked for probing */
static int chkReg[] = {
	0x00, 0x01, 0x02, 0x03,
	0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B,
	0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13,
	0x14, 0x15, 0x16, 0x17,
	-1 };


/*
 *  return 0 if not probed
 */
static	int     gl52_probe(LM_METHODS *method)
{
	int n, id, save;

	if (method != &method_smb)
		return 0;

	save = smb_slave;

	for (n = GL52_ADDR_START; n <= GL52_ADDR_END;) {
		if (!(smb_slave = get_smb_slave(n, GL52_ADDR_END)))
			goto ret0;
		else {
			id = method->Read(GL52_CHIPID);
			if ((id == 0x20 || id == 0x80)
				&& chkReg_Probe(smb_slave,
					"Probing GL518SM/GL520SM chip:\n", chkReg, method)
						>= GL52_chkRegNum)
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
	strcpy(buff, "Genesys Logic ");
	if (id == 0x20) {
		gl52chipid = GL520SM;
		temp_offset = 130;
		if (method->Read(GL52_CONF) & 0x10)
			gl52_mode = 2;
	} else {
		if (method->Read(GL52_REVNUM) == 0x80)
			gl52chipid = GL518SM_r80;
		else
			gl52chipid = GL518SM_r00;
		temp_offset = 119;
	}
	strcat(buff, gl52chip[gl52chipid]);

	return gl52chipid;
}


/*
 *	\retval	0xFFFF	no sensor
 *	\retval	other	temperature
 *  no = 0,1,...
 */
static	float	gl52_temp( LM_METHODS *method, int no )
{
	if (no < 0 || 2 < no)
		return 0xFFFF;

	if (no == 0)
		return (float) (method->Read(GL52_TEMP1) - temp_offset);
	else if (no == 1 && gl52chipid == GL520SM && gl52_mode == 1)
		return (float) (method->Read(GL52_TEMP2) - temp_offset);
	else
		return 0xFFFF;
}


/*
 *	\retval	0x0000FFFF	no sensor
 *  no = 0,1,2,...
 */
static	float	gl52_volt(LM_METHODS *method, int no)
{
	float facp12 =  1200./286.;	/* from lm_sensors */
	float facn12 = -1200./160.;	/* from lm_sensors */

	if (no < 0 || 6 < no)
		return 0xFFFF;

	if ((no > 0 && gl52chipid == GL518SM_r00)
			|| (no > 4 &&  gl52chipid == GL518SM_r80))
		return 0xFFFF;

	switch (no) {
		case 0:
			return (float) method->Read(GL52_VIN3) * 0.019;
		case 1:
			return 0.0;
		case 2:
			return (float) method->Read(GL52_VIN1) * 0.019;
		case 3:
			return (float) method->Read(GL52_VIN0) * 0.023;
		case 4:
			return (float) method->Read(GL52_VIN2) * 0.019 * facp12;
		case 5:
			if (gl52_mode == 2)
				return (float) method->Read(GL52_VIN4) * 0.019 * facn12;
			else
				return 0.0;
		case 6:
			return 0.0;
		default:
			return 0xFFFF;
	}
}


/*
	Controlling Fan Divisor: CR = 0x0F.
	highest 2bits for fan1, next 2bits for fan1.

         7     4       0
        +-+-+-+-+-+-+-+-+     xx = 00,..,11  div1fac = 1,..,8
        |x x|y y|       |     yy = 00,..,11  div2fac = 1,..,8
        +-+-+-+-+-+-+-+-+    initial values: xx=11, yy=11

	Extra number of pulse per revolution is necessary!

 */

/*
 *	\retval	0x0000FFFF	no sensor
 *  no = 0,1,...
 *
 *  Clock is 16kHz (16,000 x 60 = 960000 counts/minute)
 */
static	int		gl52_fanrpm(LM_METHODS *method, int no)
{
	int p = 2;	/* default number of pulse per revolution */
	int	r, n;
	static int div[2] = {3,3};

	if (no < 0 || 1 < no)
		return 0xFFFF;

	n = method->Read(GL52_FANDIV);
	div[0] =  n >> 6;
	div[1] = (n >> 4) & 0x03;

	r = method->ReadW(GL52_FANW);
	if (no == 0)
		r &= 0xFF;
	else
		r >>= 8;

	if (r == 0xFF) {
		/* change divisor for the sake of next call ! */
		if (div[no] < 3)
			++(div[no]);
		else
			div[no] = 0;
		r = (n & 0x0F) | (div[0] << 6) | (div[1] << 4);
		method->Write(GL52_FANDIV, r);
		return 0xFFFF;
	} else if (r == 0) {
		return 0xFFFF;
	}

	return 960000 / (r * (1 << div[no]) * p);
}
