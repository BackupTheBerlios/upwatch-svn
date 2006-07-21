
*** Brief Summary of sensor chip modules and SMBus access modules ***
    (for hackers to include supports for new hardwares!)


**************************************************************************
[1] sensor chip modules
**************************************************************************

  The treatments of reading data are different in each class of hardware
monitor chips.  They are separated into the following modules:

   sens_winbond.c   --- LM78/79, Winbond W83XXX, Asus AS99127F ASB100
   sens_wl784.c     --- Winbond W83L784X, W83L785X
   sens_via68.c     --- VIA82C686A/B
   sens_it87.c      --- ITE IT85XX and SiS950
   sens_gl52.c      --- GL518SM/GL520SM
   sens_lm85.c      --- LM85, ADM1027/ADT7463, EMC6D10X
   sens_lm80.c      --- LM80
   sens_lm90.c      --- LM90, ADM1020/1021/1023
   sens_lm75.c      --- LM75

  If one wants to add a support for new monitor chips, first prepare
the corresponding module:

   sens_xxxx.c
   sens_yyyy.c

  Then, one has to change the file, "sensors.h", in the following way
to add the new module:
-----------------------------------------------------------------------

typedef struct hwm_access SENSOR;

extern SENSOR winbond;
extern SENSOR wl784;
extern SENSOR via686;
extern SENSOR it87;
extern SENSOR gl52;
extern SENSOR lm85;
extern SENSOR lm80;
extern SENSOR lm90;
extern SENSOR lm75;
extern SENSOR xxx;   <--- add new one xxx
extern SENSOR yyy;   <--- add new one yyy

/* should be larger than the number of "HWM_sensor_chip" */
#define SEARCH	2002

/*
 *  Supported HWM, ordering is important!!
 *  HWM_sensor_chip{} should be consistent with
 *  HWM_module[] and HWM_name[]
 */

enum HWM_sensor_chip {
	c_winbond,
	c_wl784,
	c_via686,
	c_it87,
	c_gl52,
	c_lm85,
	c_lm80,
	c_lm90,
	c_lm75,
	c_xxxx,   <--- add new one xxx
	c_yyyy    <--- add new one yyy
};

#ifdef INCLUDE_HWM_MODULE

/* Array of Supported HWM, ordering is important!! */
SENSOR *HWM_module[] = {
	&winbond,
	&wl784,
	&via686,
	&it87,
	&gl52,
	&lm85,
	&lm80,
	&lm90,
	&lm75,
	&xxx,   <--- add new one xxx
	&yyy,   <--- add new one yyy
	NULL };

/* HWM_name[] should have one-to-one correspondence to HWM_module[] */
char *HWM_name[] = {
	"winbond",
	"wl784",
	"via686",
	"it87",
	"gl52",
	"lm85",
	"lm80",
	"lm90",
	"lm75",
	"xxx",   <--- add new one xxx
	"yyy",   <--- add new one yyy
	NULL };

int HWM_VIA = 0;

/* HWM_SMBchip[] should have one-to-one correspondence to HWM_module[] */
int HWM_SMBchip[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,   <--- add new one xxx
	0,   <--- add new one yyy
	0 };

/* HWM_smbslave[] should have one-to-one correspondence to HWM_module[] */
int HWM_smbslave[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,   <--- add new one xxx
	0,   <--- add new one yyy
	0 };

/* HWM_ISAchip[] should have one-to-one correspondence to HWM_module[] */
int HWM_ISAchip[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,   <--- add new one xxx
	0,   <--- add new one yyy
	0 };


#endif  /* INCLUDE_HWM_MODULE */
-----------------------------------------------------------------------

Here, the global variable of data type SENSOR, xxx and yyy, should be
defined in each module, sens_xxx.c and sens_yyy.c.

  And do not forget to add it to CHIPLIST macro in "mbmon.h".

#define CHIPLIST "winbond|wl784|via686|it87|gl52|lm85|lm80|lm90|lm75"

  As for how to make each module, please refer existing ones above.



**************************************************************************
[2] smbus modules
**************************************************************************

  The treatments of SMBus access are different for various Power Management
Controllers (usually inside the south chipset).  They are separated into
the following modules:

   smbus_piix4.c  --- PIIX4(440BX), VT82C586/596/686A/B(KT133 etc),
                      VT8233(KT266/A, KT333), etc 
   smbus_amd.c    --- AMD756/766/768, NVidia nForce
   smbus_ali.c    --- ALi M1535D+
   smbus_amd8.c   --- AMD8111, NVidia nForce2

  If one wants to add a support for new PM chipset to access SMBus, first
prepare the corresponding module:

   smbus_xxx.c
   smbus_yyy.c

in each of which,

   struct smbus_io smbus_xxxx = ....
or
   struct smbus_io smbus_yyyy = ....

should be defined.  This structure "smbus_xxxx/yyyy" is used in
"smbuses.c" below.

  Next, one has to make the PM chip to be recognized in the search routine
of the PCI Configuration in the files.  For this purpose one has to modify
the file "pci_pm.c" and "pci_pm.h".  Include new macros in pci_pm.h,

   #define XXxxxSMB	xx(int, appropriate number)
   #define YYyyySMB	yy(int, appropriate number)

and correspondingly change the file pci_pm.c.  Here one needs the following
information:

   (a)The PCI chip and vendor ID of the PM chip to be supported
   (b)The offset where the SMBus base address is stored

For the supported chipsets, these information are included in pci_pm.h.
Refer to it for adding new PM chipsets.

  Then one has to change the file, "smbuses.c", in the following way
 to add the module:
-----------------------------------------------------------------------
/* smbus io routines, global */
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
#else		/* using own SMBus IO routines */
extern SMBUS_IO *smbus;
extern SMBUS_IO smbus_piix4;
extern SMBUS_IO smbus_amd;
extern SMBUS_IO smbus_ali;
extern SMBUS_IO smbus_amd8;
extern SMBUS_IO smbus_xxx;   <--- add new one xxx
extern SMBUS_IO smbus_yyy;   <--- add new one yyy
#endif

int set_smbus_io(int *viahwm_base, int *smb_base)
{
	int n;

	n = pci_pm_smb_prob(viahwm_base, smb_base);
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
	return n;
#else           /* using own SMBus IO routines */
	if (n <= 0)
		return n;

	if (n/10 == AMD756SMB/10)
		smbus = &smbus_amd;
	else if (n/10 == ALI7101SMB/10)
		smbus = &smbus_ali;
	else if (n/10 == AMD8111SMB/10)
		smbus = &smbus_amd8;
	else if (n/10 == XXxxxSMB/10)   <--- add new one xxx
			smbus = &smbus_xxx;
	else if (n/10 == YYyyySMB/10)   <--- add new one yyy
		smbus = &smbus_yyy;
	else
		smbus = &smbus_piix4;

	return n;
#endif
}

