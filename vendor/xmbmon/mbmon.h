/*
 * Yoshifumi R. Shimizu
 *
 * ver.1.00, 1999.02.01-04
 *     Using only Xlib
 *
 * ver.1.01, 1999.02.06
 *     Using X Toolkit
 *
 * ver.1.02, 1999.02.09
 *     Using application resources and option tables
 *     Winbond W83781D supported.
 *     National Semiconductor LM78/79 supported (possibly).
 *
 * ver.1.03, 1999.02.20-23
 *     Adding temp3, and increasing resouces / option tables
 *     -DNO_TEMP3 compiling option for not showing temp3
 *
 * ver.1.04, 1999.05.04
 *     Slight improvement for getting resources
 *
 * ver.1.05, 1999.11.13
 *     Support SMBus access method.  Debugging infomation.
 *     Winbond W83781D, W83782D, W83783S supported.
 *     Asus AS99127F HWM chip supported.
 *
 * ver.1.06, 2001.08.22
 *     New SMBus access method without "intpm" driver!
 *     VIA VT82C686A/B's HWM chip supported.
 *
 * ver.1.07, 2002.01.16
 *     Patch for supporting detailed output for MRTG,
 *     provided by Koji MORITA (morita@cybird.co.jp).
 *     Showing temperture in Fahrenheit.
 *
 * ver.2.00, 2002.07.31
 *     Supported chips' codes are separated into modules.
 *     National Semiconductor LM80 moudule contributed
 *     by Shin-ichi Nagamura.
 *     ASUS ASB100 chip is supported.
 *     National Semiconductor LM75 supported (possibly).
 *     SMBus access routines are also separated into modules.
 *     New SMBus access routines for AMD7xx and ALi7101.
 *
 * ver.2.01, 2003.01.20
 *     New option to execute "mbmon" as a daemon contributed
 *     by Jean-Marc Zucconi (jmz@FreeBSD.org).
 *     ASUS Mozart-2 sensor chip is supported.
 *     Special treatment of ASUS ASB100.
 *
 * ver.2.02, 2003.06.15
 *     Supporting patch of NetBSD/OpenBSD (by Stephan Eisvogel).
 *     New SMBus access for AMD8111 and NVidia nForce2 chipsets.
 *     LM90 sensor chip is supported.
 *     Winbond W83L784R, W83L785R, W83L785TS-S are supported.
 *     The case of two sensor chips, i.e. with an extra-CPU
 *     temperature sensor for "CPU thermal protection", is supported!
 *
 * ver.2.03, 2003.06.25, 07.04, 07.30
 *     Fixing bugs about fan-divisor, clean up code
 *     (no need & 0xFF for data by ReadByte).
 *     Gensys Logic GL518SM/GL520SM chips are supported.
 *     LM85 and compatible, Analog Devices ADM1024/1025/1027/ADT7463,
 *     SMSC EMC6D100/101 chips are supported.
 *     Analog Devices ADM1020/1021/1023 temperature sensors
 *     are supported (LM90 compatible).
 *
 * ver.2.04, 2003.10.02, 2004.02.02, 03.31
 *     Winbond W83791D, W83627THF, W83627THF-A chips are supported.
 *     2nd order IIR Low Pass Filter (LPF) included
 *     for graphs in xmbmon by Takayuki Hosoda.
 *     ALI1533/1543 chipset are supported (change of only SMBBA).
 *     Logging facility added by by David Quattlebaum.
 *
 * ver.2.05, 2004.06.04, 07.15
 *     Bug fixed for ALI chipset detection.
 *     Abrupt change of values in xmbmon, which causes spikes of
 *     curves, are avoided.
 *     VIA VT8237 and Intel ICH6 supported.
 *     Problem of select system call fixed (by Tsuneyuki Sakano).
 *     IO port read/write code replaced by x86-gas inline assembler
 *     (by John Wehle).
 *     Supporting patch for Solaris on x86 (by John Wehle).
 *
 *
 *  mbmon  --- command-line motherboard monitor
 *
 * xmbmon  --- X motherboard monitor
 *
 *
 * << Acknowledgements >>
 *
 * Information related to WinBond W83781D LM78/LM79 Chips by Alex van Kaam.
 *  http://mbm.livewiredev.com/
 *
 * Information on VT82C686A/B chips and many general things related to
 * both hardware monitor chips and SMBus access by ":p araffin(Yoneya)".
 *  http://homepage1.nifty.com/paraffin
 *
 * Information for SMBus access by Linux lm_sensor homepage.
 *  http://www.lm-sensors.nu/
 *
 */

#if !defined(__mbmon_h__)

#define XMBMON_VERSION "2.05"

#define DEFAULT_SEC 5

#define CHIPLIST "winbond|wl784|via686|it87|gl52|lm85|lm80|lm90|lm75"

/* Fahrenheit flag used in getTemp(), global */
int fahrn_flag = 0;

/* Debug flag used in InitMBInfo(), global */
int debug_flag = 0;

/* TyanTigerMP flag,  global */
int TyanTigerMP_flag = 0;

/* detected HWM or SMB ID number, global */
int pm_smb_detected = 0;

/* Probe request characters for detecting hardware monitor chip */
char *probe_request = "none";

/* the number of temp.[0|1|2] to replace extra temperature with */
int extra_tempNO = 2;

/* functions */
int InitMBInfo(char);
int getTemp(float *, float *, float *);
int getVolt(float *, float *, float *, float *, float *, float *, float *);
int getFanSp(int *, int *, int *);


#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
/* SMBus device file name */
char smb_devbuf[] = "/dev/smb0";
char *smb_devfile = smb_devbuf;
#endif


#ifdef LOGGING
/* our circular list of log data */
typedef struct LOGDATA {
	struct LOGDATA *next;
	struct LOGDATA *prev;
	char *data;
} logdata;

/* log file name */
#if !defined(LOGFILE)
#define LOGFILE "/var/log/mbmon.log"
#endif

/* seconds between log entries */
#if !defined(LOGINTERVAL)
#define LOGINTERVAL 300
#endif

/* maximum entries in log file */
#if !defined(LOGENTRIES)
#define LOGENTRIES 1024
#endif

#endif	/* LOGGING */

#endif	/*__mbmon_h__*/
