/*
 * Integrated Technology Express IT8705F/IT8712F hardware monitor chip
 *
 ***************************************************************
 * Before calling these routines, one must call method->Open() *
 * After calling these routines, one must call method->Close() *
 ***************************************************************
 *

Integrated Technology Express
         Chip         Temp    Volt    Fan     SMBus   IOport
        it8705         3       8       3       yes     yes
        it8712         3       8       3       yes     yes

SiS
         Chip         Temp    Volt    Fan     SMBus   IOport
        sis950         3       8       3       yes     yes

 *
 * by YRS
 */


#include	<stdio.h>
#include	"sensors.h"

/* external (global) data */
extern int smb_slave;
extern LM_METHODS method_isa, method_smb;
extern int numSMBSlave, canSMBSlave[128];


#define	IT87_ADDR_START		0x50	/*0x50-0x5E*/
#define	IT87_ADDR_END		0x5E

#define	IT87_SMBADDR	0x48
#define	IT87_REGCHIP	0x58
#define	IT87_CHIPID		0x90

/* temp nr=0,1,2; volt nr=0,1,...6; fan nr=0,1,2 */
#define	IT87_TEMP(nr)	(0x29 + (nr))
#define	IT87_VOLT(nr)	(0x20 + (nr))
#define	IT87_FAN(nr)	(0x0D + (nr))
#define	IT87_FANDIV		0x0B

static	int		it87_probe(LM_METHODS *);
static	float	it87_temp(LM_METHODS *, int);
static	int		it87_fanrpm(LM_METHODS *, int);
static	float	it87_volt(LM_METHODS *, int);

SENSOR it87 = {
	"Int.Tec.Exp. Chip IT8705F/IT8712F or SIS950",
	it87_probe,
	it87_temp,
	it87_volt,
	it87_fanrpm
};


#define IT87_chkRegNum 8

/* Register checked for probing */
static int chkReg[] = {
	0x00, 0x01, 0x02, 0x03,
	0x0A, 0x48, 0x50, 0x51,
	0x20, 0x21, 0x22, 0x23,
	0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2A, 0x2B,
	0x0B, 0x0D, 0x0E, 0x0F,
	-1 };


/*
 *  return 0 if not probed
 */
static	int     it87_probe(LM_METHODS *method)
{
	int n, save;

	if (method != &method_isa  && method != &method_smb)
		return 0;

	save = smb_slave;

	if (method == &method_smb) {
		for (n = IT87_ADDR_START; n <= IT87_ADDR_END;) {
			if (!(smb_slave = get_smb_slave(n, IT87_ADDR_END)))
				goto ret0;
			else if (smb_slave != 2 * method->Read(IT87_SMBADDR))
				goto ret0;
			else {
				if (method->Read(IT87_REGCHIP) == IT87_CHIPID
					&& chkReg_Probe(smb_slave,
						"Probing ITE7805/7812/SIS950 chip:\n", chkReg, method)
							>= IT87_chkRegNum)
					goto ret1;
				else
					n = smb_slave + 2;
			}
		}
		goto ret0;
	} else {
		if (method->Read(IT87_REGCHIP) == IT87_CHIPID
			&& chkReg_Probe(0, "Probing ITE7805/7812/SIS950 chip:\n",
				chkReg, method) >= IT87_chkRegNum)
			goto ret1;
	}

ret0:
	smb_slave = save;
	return 0;
ret1:
	if (method == &method_smb)
		kill_smb_slave(smb_slave);
	return 1;
}


/*
 *	\retval	0xFFFF	no sensor
 *	\retval	other	temperature
 *  no = 0,1,2,...  
 */
static	float	it87_temp( LM_METHODS *method, int no )
{
	if (no < 0 || 2 < no)
		return 0xFFFF;

	return (float) method->Read(IT87_TEMP(no));
}


/*
 *	\retval	0x0000FFFF	no sensor
 *  no = 0,1,2,...  
 */
static	float	it87_volt(LM_METHODS *method, int no)
{
	float fac;

	if (no < 0 || 6 < no)
		return 0xFFFF;

	switch (no) {
		case 0:
		case 1:
		case 2:
			fac = 0.016;
			break;
		case 3:
			fac = 0.016 * 1.68;
			break;
		case 4:
			fac = 0.016 * 3.80;
			break;
		case 5:
			fac = - 0.016 * 3.477;
			break;
		case 6:
			fac = - 0.016 * 1.505;
	}

	return (float) method->Read(IT87_VOLT(no)) * fac;
}


/*
	Controlling Fan Divisor for 1st/2nd fans: CR = 0x0B.
	lowest three bits for fan1, next three bits for fan2

         7       3     0
        +-+-+-+-+-+-+-+-+     xxx = 000,..,111  div1fac = 1,..,128
        |   |y y y|x x x|     yyy = 000,..,111  div2fac = 1,..,128
        +-+-+-+-+-+-+-+-+    initial values: xx=001, yy=001

	No divisor available for fan3.

 */

/*
 *	\retval	0x0000FFFF	no sensor
 *  no = 0,1,2,...  
 */
static	int		it87_fanrpm(LM_METHODS *method, int no)
{
	int	r, n;
	static int div[3] = {1,1,1};

	if (no < 0 || 2 < no)
		return 0xFFFF;

	n = method->Read(IT87_FANDIV);
	div[0] =  n & 0x07;
	div[1] = (n >> 3) & 0x07;

	r = method->Read(IT87_FAN(no));
	if (r == 0xFF) {
		/* change divisor for the sake of next call ! */
		if (no != 2) {
			if (div[no] < 7)
				++(div[no]);
			else
				div[no] = 0;
			r = (n & 0x3F) | div[0] | (div[1] << 3);
			method->Write(IT87_FANDIV, r);
		}
		return 0xFFFF;
	} else if (r == 0) {
		return 0xFFFF;
	}

	return 1350000 / (r * (1 << div[no]));
}
