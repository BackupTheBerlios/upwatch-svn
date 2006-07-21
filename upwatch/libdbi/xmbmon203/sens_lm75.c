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
        lm75           1       -       -       yes     no

          (the following chips are detected as an lm75)
         Chip         Temp    Volt    Fan     SMBus   IOport
        ds75           1       -       -       yes     no
        ds1775         1       -       -       yes     no
        tcn75          1       -       -       yes     no
        lm77           1       -       -       yes     no

 *
 * by YRS
 */


#include	<stdio.h>
#include	"sensors.h"

/* external (global) data */
extern int smb_slave;
extern int smb_wbtemp1, smb_wbtemp2;
extern LM_METHODS method_smb;
extern int numSMBSlave, canSMBSlave[128];


#define	LM75_ADDR_START		0x90	/*0x90-0x9E*/
#define	LM75_ADDR_END		0x9E

static	int		lm75_probe(LM_METHODS *);
static	float	lm75_temp(LM_METHODS *, int);
static	int		lm75_fanrpm(LM_METHODS *, int);
static	float	lm75_volt(LM_METHODS *, int);

SENSOR lm75 = {
	"Nat.Semi.Con. Chip LM75",
	lm75_probe,
	lm75_temp,
	lm75_volt,
	lm75_fanrpm
};


/* temp1/2 flags address*/
static int temp1_flag = 0;	/* = 0 if enabled ! */
static int temp2_flag = 0;	/* = 0 if enabled ! */

/*
 *  return 0 if not probed
 */
static	int     lm75_probe(LM_METHODS *method)
{
	int i, j, k, save;

	if (method != &method_smb)
		return 0;

	i = set_smb_Extemp(LM75_ADDR_START, LM75_ADDR_END,
			&smb_wbtemp1, &smb_wbtemp2);
	temp1_flag = i >> 1;
	temp2_flag = i & 0x01;

	if (temp1_flag && temp2_flag)
		return 0;

	save = smb_slave;
	smb_slave = smb_wbtemp1;
	i = method->Read(0x01);
	if (i > 0x1F)
		goto ret0;
	i = method->Read(0x00);
	j = method->Read(0x02);
	k = method->Read(0x03);
	if (j == 0xFF || j == 0 || k == 0xFF || k == 0 ||
		(i == j && i == k))
		goto ret0;
#if DEBUG
	printf("DEBUG: 0x00=%d, 0x02=%d, 0x03=%d\n", i, j, k);
#endif
	/* requiring j(0x02)=OS temp >= 40 deg.C, k(0x03)=HYST temp >= 20 */
	if (j < 40 || k < 20)
		goto ret0;

	info_Extemp(method, temp1_flag, temp2_flag);

	kill_smb_slave(smb_slave);
	return 1;
ret0:
	smb_slave = save;
	return 0;
}


/*
 *	\retval	0xFFFF	no sensor
 *	\retval	other	temperature
 *  no = 0,1,2,...  
 */
static	float	lm75_temp( LM_METHODS *method, int no )
{
	int n = 0;

	if (no < 0 || 1 < no)
		return 0xFFFF;

	if (no == 0 && !temp1_flag)
		n = method->ReadTemp1();
	else if (no == 1 && !temp2_flag)
		n = method->ReadTemp2();
	if ((n & 0xFF) >= 0x80)
		n = 0;
	return ((float) (n & 0xFF) + 0.5 * ((n & 0xFF00) >> 15));
}

/* lm75 is only for temperature sensor */

static	float	lm75_volt(LM_METHODS *method, int no)
{
	return 0xFFFF;
}

static	int		lm75_fanrpm(LM_METHODS *method, int no)
{
	return 0xFFFF;
}

