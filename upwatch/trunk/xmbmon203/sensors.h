#if !defined(__sensor_h__)
#define	__sensor_h__

#include "methods.h"

struct hwm_access {
	char	*Name;
	int		(*Probe)(LM_METHODS *pMethods);
	float	(*Temp)(LM_METHODS *pMethods, int no);
	float	(*Volt)(LM_METHODS *pMethods, int no);
	int		(*FanRPM)(LM_METHODS *pMethods, int no);
};

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

/* should be larger than the number of "HWM_sensor_chip" */
#define SEARCH	2002

/*
 *	Supported HWM, ordering is important!!
 *	HWM_sensor_chip{} should be consistent with
 *	HWM_module[] and HWM_name[]
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
	c_lm75
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
	NULL };

/* number of VIA device found */
int HWM_VIA = 0;

/* number of SMB device found */
int HWM_SMB = 0;

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
	0 };

/* number of ISA device found */
int HWM_ISA = 0;

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
	0 };

#endif	/* INCLUDE_HWM_MODULE */


/* functions commonly used in each HWM module */
extern int chkReg_Probe(int slave, char *comment, int Reg[], LM_METHODS *);
extern int strict_chkReg_Probe(int Reg[], LM_METHODS *method);
extern int scan_smbus(int addr_start, int addr_end, int result[]);
extern int find_smb_dev(void);
extern void kill_smb_slave(int slave);
extern int get_smb_slave(int start, int end);
extern int set_smb_Extemp(int start, int end, int *slavet1, int *slavet2);
extern void info_Extemp(LM_METHODS *, int, int);

#endif	/*!__sensor_h__*/
