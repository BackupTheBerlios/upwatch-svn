/*
 * VIA VT82C686A/B hardware monitor chip
 *
 ***************************************************************
 * Before calling these routines, one must call method->Open() *
 * After calling these routines, one must call method->Close() *
 ***************************************************************
 *

VIA
         Chip         Temp    Volt    Fan     SMBus   IOport
        via686a        3       5       2       yes     yes

 *
 * by YRS
 */


#include "pci_pm.h"
#include "sensors.h"

/* external (global) data */
extern int smb_slave;
extern int pm_smb_detected;
extern LM_METHODS method_via, method_smb;
extern int numSMBSlave, canSMBSlave[128];


#define	VIA_ADDR_START		0x50	/*0x50-0x5E*/
#define	VIA_ADDR_END		0x5E

/* temp nr=0,1,2; volt nr=0,...4; fan nr=0,1 */
#define	VIA686_TEMP(nr)	(0x1F + (nr))
#define	VIA686_VOLT(nr)	(0x22 + (nr))
#define	VIA686_FAN(nr)	(0x29 + (nr))
#define	VIA686_FANDIV	0x47

static	int		via686_probe(LM_METHODS *methods);
static	float	via686_temp(LM_METHODS *methods, int no);
static	int		via686_fanrpm(LM_METHODS *methods, int no);
static	float	via686_volt(LM_METHODS *methods, int no);

 SENSOR via686 = {
	"VIA Chip VT82C686A/B",
	via686_probe,
	via686_temp,
	via686_volt,
	via686_fanrpm
};


#define VIA_chkRegNum 5

/* Register checked for probing */
static int chkReg[] = {
	0x40, 0x41, 0x42, 0x43,
	0x44, 0x47, 0x49, 0x4B,
	0x3F, 0x14, 0x1F, 0x20,
	0x21, 0x22, 0x23, 0x24,
	0x25, 0x26, 0x29, 0x2B,
	-1 };


/*
 *  return 0 if not probed
 */
static	int     via686_probe(LM_METHODS *method)
{
	int n, save;

	if (method != &method_via && method != &method_smb)
		return 0;

  	if (pm_smb_detected != VIA686HWM && pm_smb_detected !=  VIA686SMB)
		return 0;

	save = smb_slave;

	if (method == &method_smb) {
		for (n = VIA_ADDR_START; n <= VIA_ADDR_END;) {
			if (!(smb_slave = get_smb_slave(n, VIA_ADDR_END)))
				goto ret0;
			else {
				if (chkReg_Probe(smb_slave, "Probing VIA686A/B chip:\n",
						chkReg, method) >= VIA_chkRegNum)
					goto ret1;
				else
				n = smb_slave + 2;
			}
		}
	} else {
		if (chkReg_Probe(0, "Probing VIA686A/B chip:\n",
					chkReg, method) >= VIA_chkRegNum)
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


static const float via686temp_tab[256] = {\
   .00,   .00,   .00,   .00,   .00,   .00,   .00,   .00,
   .00,   .00,   .00,   .00,   .00,   .00,   .00,-50.00,
-48.00,-46.09,-44.26,-42.52,-40.86,-39.28,-37.78,-36.35,
-34.99,-33.70,-32.47,-31.31,-30.20,-29.15,-28.14,-27.18,
-26.27,-25.39,-24.55,-23.73,-22.95,-22.18,-21.44,-20.70,
-19.98,-19.27,-18.57,-17.87,-17.19,-16.51,-15.85,-15.20,
-14.56,-13.93,-13.32,-12.72,-12.14,-11.58,-11.03,-10.50,
 -9.99, -9.50, -9.03, -8.57, -8.11, -7.67, -7.22, -6.77,
 -6.31, -5.85, -5.37, -4.87, -4.36, -3.81, -3.24, -2.64,
 -2.00, -1.33,  -.65,   .00,   .59,  1.12,  1.60,  2.03,
  2.42,  2.79,  3.12,  3.44,  3.75,  4.05,  4.35,  4.67,
  4.99,  5.34,  5.71,  6.09,  6.49,  6.91,  7.34,  7.78,
  8.23,  8.69,  9.15,  9.62, 10.09, 10.57, 11.04, 11.51,
 11.98, 12.44, 12.90, 13.35, 13.80, 14.24, 14.68, 15.12,
 15.55, 15.99, 16.42, 16.85, 17.27, 17.70, 18.13, 18.56,
 18.99, 19.42, 19.85, 20.29, 20.72, 21.15, 21.59, 22.03,
 22.47, 22.90, 23.34, 23.78, 24.22, 24.66, 25.10, 25.54,
 25.99, 26.43, 26.87, 27.31, 27.75, 28.19, 28.63, 29.07,
 29.51, 29.95, 30.38, 30.82, 31.25, 31.68, 32.11, 32.53,
 32.96, 33.38, 33.80, 34.22, 34.63, 35.05, 35.48, 35.90,
 36.33, 36.76, 37.20, 37.64, 38.09, 38.55, 39.02, 39.50,
 39.99, 40.49, 41.00, 41.52, 42.06, 42.60, 43.15, 43.70,
 44.27, 44.84, 45.41, 45.99, 46.57, 47.16, 47.75, 48.35,
 48.94, 49.54, 50.13, 50.73, 51.33, 51.94, 52.55, 53.16,
 53.78, 54.41, 55.04, 55.68, 56.32, 56.98, 57.64, 58.32,
 59.00, 59.69, 60.40, 61.12, 61.85, 62.60, 63.36, 64.13,
 64.93, 65.73, 66.56, 67.40, 68.26, 69.14, 70.05, 70.97,
 71.91, 72.88, 73.86, 74.88, 75.92, 76.98, 78.08, 79.21,
 80.36, 81.55, 82.78, 84.03, 85.33, 86.66, 88.04, 89.45,
 90.90, 92.40, 93.94, 95.53, 97.16, 98.85,100.58,102.36,
104.19,106.07,108.01,110.00,   .00,   .00,   .00,   .00,
   .00,   .00,   .00,   .00,   .00,   .00,   .00,   .00
};

/*
 *	\retval	0xFFFF	no sensor
 *	\retval	other	temperature
 *  no = 0,1,2,...
 */
static	float	via686_temp(LM_METHODS *method, int no)
{
	if (no < 0 || 2 < no)
		return 0xFFFF;

	return via686temp_tab[method->Read(VIA686_TEMP(no))];
}


/*
 *	\retval	0x0000FFFF	no sensor
 *  no = 0,1,2,...
 */
static	float	via686_volt(LM_METHODS *method, int no)
{
	int n;
	float fac;

	if ( no < 0 || 4 < no )
		return 0xFFFF;

	switch (no) {
		case 0:
		case 1:
			fac = 5.  / 10512.;
			break;
		case 2:
			fac = 5.  /  7884.;
			break;
		case 3:
			fac = 26. / 26280.;
			break;
		case 4:
			fac = 63. / 26280.;
	}

	if ((n = method->Read(VIA686_VOLT(no))) <= 0x40)
		n |= 0x100;

	return (float) (n * 25 + 133) * fac;
}


/*
	Controlling Fan Divisor: CR = 0x47.
	highest two bits for fan2, next two bits for fan1

         7       3     0
        +-+-+-+-+-+-+-+-+     xx = 00,01,10,11  div1fac = 1,2,4,8
        |y y|x x| VolID |     yy = 00,01,10,11  div2fac = 1,2,4,8
        +-+-+-+-+-+-+-+-+    initial values: xx=01, yy=01

 */

/*
 *	\retval	0x0000FFFF	no sensor
 *  no = 0,1,2,...
 *
 *  Clock is 22.5kHz (22,500 x 60 = 1350000 counts/minute)
 */
static	int		via686_fanrpm(LM_METHODS *method, int no)
{
	int r, n;
	static int div[2] = {1,1};

	if (no < 0 || 1 < no)
		return 0xFFFF;

	n = method->Read(VIA686_FANDIV);
	div[0] = (n >> 4) & 0x03;
	div[1] =  n >> 6;

	r = method->Read(VIA686_FAN(no));
	if (r == 0xFF) {
		/* change divisor for the sake of next call ! */
		if (div[no] < 3)
			++(div[no]);
		else
			div[no] = 0;
		r = (n & 0x0F) | (div[0] << 4) | (div[1] << 6);
		method->Write(VIA686_FANDIV, r);
		return 0xFFFF;
	} else if (r == 0) {
		return 0xFFFF;
	}

	return 1350000 / (r * (1 << div[no]));
}
