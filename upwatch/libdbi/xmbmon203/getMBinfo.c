/************************************************
	Subroutines to get Mother Board Information

	Information related to Winbond W83781D Chip
		and National Semiconductor LM78/LM79 Chips
	by Alex van Kaam

	Information for VIA VT82C686A/B
	by ":p araffin.(Yoneya)", MANY THANKS!!

	Information for SMBus access
	by Linux lm_sensor homepage, MANY THANKS!!
		http://www.netroedge.com/~lm78/

 ************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef LINUX	/* LINUX */
#include <string.h>
#endif
#include <stdio.h>
#include "pci_pm.h"
#include "smbuses.h"
#include "methods.h"
#include "sensors.h"
#include "smb_extemp.h"

#ifdef TEMP_LIMIT
#define TEMP_HIGH 100.0
#define TEMP_LOW  -10.0
#endif

/* external (global) data */
extern int debug_flag, fahrn_flag;
extern int TyanTigerMP_flag;
extern int isa_port_base;
extern int viahwm_base, smb_base;
extern int pm_smb_detected;
extern int smb_slave;
extern char *probe_request;
extern SENSOR *HWM_module[];
extern char *HWM_name[];
extern int HWM_VIA, HWM_SMB, HWM_ISA;
extern int HWM_SMBchip[];
extern int HWM_smbslave[];
extern int HWM_ISAchip[];
extern int numSMBSlave;
extern int canSMBSlave[];
extern int num_extemp_chip;
extern int smb_extemp_chip[];
extern int smb_extemp_slave[];
extern int extra_tempNO;

/* Access method functions, global */
extern LM_METHODS method_isa, method_smb, method_via;


static LM_METHODS *this_method = NULL;
static SENSOR *this_sensor = NULL;
static int probe_flag = SEARCH;
static int HWM_firstSMB_flag = 0;
static char method;

/* function declarations */
extern void TyanTigerMPinit(void);
int Probe_method(void);
int HWM_detection(int);
void HWM_set_firstSMB(SENSOR *, int);
int probe_HWMChip(LM_METHODS *, int);
int via_set(void);
int smb_set(int, int);
int isa_set(int);
int InitMBInfo(char);
int getTemp(float *, float *, float *);
int getVolt(float *, float *, float *, float *, float *, float *, float *);
int getFanSp(int *, int *, int *);


/*----------------------
	Detecting HWM Chip
  ----------------------*/

int Probe_method(void)
{
	int n;

	if (method != 'I') {
		pm_smb_detected = set_smbus_io(&viahwm_base, &smb_base);
	}
	if (method == 'V') {
  		if ((n = pm_smb_detected) == VIA686HWM) {
		/* VIA VT82C686 HWM is available */
			return via_set();
		} else if (n != -1) {
			fprintf(stderr, "No VIA686 HWM available!!\n");
			return 1;
		}
	} else if (method == 'S') {
  		if ((n = pm_smb_detected) > 0) {
		/* SMBus PowerManagement, hardware monitor exist ? */
			return smb_set(probe_flag, n);
		} else if (n != -1) {
			fprintf(stderr, "No SMBus HWM available!!\n");
			return 1;
		}
	} else if (method == 'I') {
		/* Just try ISA-IO method */
		if ((n = isa_set(probe_flag)) == 0) {
			return 0;
		} else if (n != -1) {
			fprintf(stderr, "No ISA-IO HWM available!!\n");
			return 1;
		}
	} else {
	/* No input method option: Try probing each HWM type */
	  	if ((probe_flag == SEARCH || probe_flag == c_via686)
					&& (n = pm_smb_detected) == VIA686HWM) {
			if (via_set() == 0 && method != 'A')
				return 0;
			else if (n > 0)
				goto smb_chk;
		} else if ((n = pm_smb_detected) > 0) {
smb_chk:	if (smb_set(probe_flag, n) == 0 && method != 'A')
				return 0;
			goto isa_chk;
		} else {
isa_chk:	if ((n = isa_set(probe_flag)) == 0 && method != 'A')
				return 0;
			else if (n != -1 && method != 'A') {
				fprintf(stderr, "No Hardware Monitor found!!\n");
				return 1;
			}
		}
	}

	if (method == 'A') {
		return HWM_detection(pm_smb_detected);
	}

	return -1;
}

int HWM_detection(int chip)
{
	int n, j, k, ext;

	if (debug_flag)
		fprintf(stderr, "Summary of Detection:\n");

	if (HWM_VIA + HWM_SMB + HWM_ISA <= 0) {
		if (debug_flag)
			fprintf(stderr, " * No monitors found.\n");
		return -1;
	}

	if (HWM_VIA > 0) {
		this_method = &method_via;
		if (debug_flag)
			fprintf(stderr, " * VIA686A/B monitor found.\n");
	}
	for (n = 0, k = 0; HWM_module[n] != NULL; n++) {
		if (HWM_SMBchip[n] != 0) {
			if (debug_flag) {
				if (!k)
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
					fprintf(stderr, " * SMB monitor(s)[ioctl:%s]:\n",
#else		/* SMBus direct access routines */
					fprintf(stderr, " * SMB monitor(s)[%s]:\n",
#endif
						chk_smb_chip(chip));
				fprintf(stderr, "  ** %s found at slave address: 0x%02X.\n",
					HWM_module[n]->Name, HWM_smbslave[n]);
			}
			k++;
			ext = 0;
			j = num_extemp_chip;
			if (!strcmp(HWM_name[n], "lm75")) {
				smb_extemp_chip[j] = ex_lm75;
				ext = 1;
			} else if (!strcmp(HWM_name[n], "lm90")) {
				smb_extemp_chip[j] = ex_lm90;
				ext = 1;
			} else if (!strcmp(HWM_name[n], "wl784")
						&& HWM_SMBchip[n] == W83L785TS) {
				smb_extemp_chip[j] = ex_wl785ts;
				ext = 1;
			}
			if (ext) {
				smb_extemp_slave[j] = HWM_smbslave[n];
				num_extemp_chip++;
			}
			/* set the first SMB monitor found */
			if (!ext && !HWM_firstSMB_flag && HWM_SMB > 1)
				HWM_set_firstSMB(HWM_module[n], HWM_smbslave[n]);
		}
	}
	for (n = 0, k = 0; HWM_module[n] != NULL; n++) {
		if (HWM_ISAchip[n] != 0) {
			if (debug_flag) {
				if (!k)
					fprintf(stderr, " * ISA monitor(s):\n");
				fprintf(stderr, "  ** %s found.\n", HWM_module[n]->Name);
			}
			k++;
		}
	}
#ifdef DEBUG
printf("HWM_VIA=%d, HWM_SMB=%d, HWM_ISA=%d\n", HWM_VIA, HWM_SMB, HWM_ISA);
#endif
	if (HWM_VIA + HWM_SMB + HWM_ISA == num_extemp_chip)
		num_extemp_chip = 0;

	return 0;
}

void HWM_set_firstSMB(SENSOR *module, int slave)
{
	int n;

	for (n = 0; n < numSMBSlave; n++) {
		if (canSMBSlave[n] == 0xFF) {
			canSMBSlave[n] = slave;
			break;
		}
	}

	this_method = &method_smb;
	this_sensor = module;

	n = debug_flag;
	debug_flag = 0;
	{
		this_method->Open();
		this_sensor->Probe(this_method);
		this_method->Close();
	}
	debug_flag = n;
	HWM_firstSMB_flag = 1;
}

int probe_HWMChip(LM_METHODS *methodp, int probe)
{
	int i, n, n0 = 0, num = 0;

	if (methodp->Open() != 0)
		return -1;

	/* module order is important !! */
	for (n = 0; HWM_module[n] != NULL; n++) {
		if ((probe == SEARCH || probe == n)
				&& (i = HWM_module[n]->Probe(methodp)) != 0) {
			if (methodp == &method_smb) {
				HWM_SMBchip[n] = i;
				HWM_smbslave[n] = smb_slave;
				HWM_SMB++;
			} else if (methodp == &method_isa) {
				HWM_ISAchip[n] = i;
				HWM_ISA++;
			} else {
				HWM_VIA++;
			}
			num++;
			if (n0 == 0)
				n0 = n;
			if (method != 'A')
				break;
		}
	}
	if (num)
		this_sensor = HWM_module[n0];	/* set HWM found first */

	methodp->Close();
	return num;
}

int via_set(void)
{
	if (debug_flag > 1)
		fprintf(stderr, ">>> Testing Reg's at VIA686 HWM <<<\n");

	if (probe_HWMChip(&method_via, c_via686) > 0) {
		this_method = &method_via;
		if (debug_flag && method != 'A')
			fprintf(stderr, "Using VIA686 HWM directly!!\n");
		return 0;
	} else {
		fprintf(stderr, "Something Wrong in detected VIA686 HWM!!\n");
		return 1;
	}
}

int smb_set(int probe, int chip)
{
	if (debug_flag > 1) {
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
		fprintf(stderr, ">>> Testing Reg's at SMBus <<<\n");
#else		/* SMBus direct access routines */
		fprintf(stderr, ">>> Testing Reg's at SMBus <<<\n"\
			"[%s, IO-Base:0x%0X]\n", chk_smb_chip(chip), smb_base);
#endif
	}
	if (find_smb_dev() <= 0)
		goto ret1;

	if (probe_HWMChip(&method_smb, probe) > 0) {
		this_method = &method_smb;
		if (debug_flag && method != 'A') {
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
			fprintf(stderr, "Using SMBus-ioctl access method[%s]!!\n",
#else		/* SMBus direct access routines */
			fprintf(stderr, "Using SMBus access method[%s]!!\n",
#endif
				chk_smb_chip(chip));
		}
		return 0;
	} else {
ret1:	if (debug_flag) {
			fprintf(stderr, "SMBus[%s] found, but No HWM available on it!!\n",
				chk_smb_chip(chip));
		}
		return 1;
	}
}

int isa_set(int probe)
{
	int n;

	if (debug_flag > 1)
		fprintf(stderr, ">>> Testing Reg's at ISA-IO <<<\n"\
			"[ISA Port IO-Base:0x%0X]\n", isa_port_base);

	if ((n = probe_HWMChip(&method_isa, probe)) > 0) {
		this_method = &method_isa;
		if (debug_flag && method != 'A')
			fprintf(stderr, "Using ISA-IO access method!!\n");
		return 0;
	} else if (n == 0)
		n = 1;

	return n;
}

int InitMBInfo(char method_inp)
{
	int n;

 	/* this is TyanTigerMP specific treatment */
	if (TyanTigerMP_flag)
	 	TyanTigerMPinit();

	if (debug_flag > 1)
		fprintf(stderr, "Probe Request: %s\n", probe_request);

	for (n = 0; HWM_name[n] != NULL; n++) {
		if (strcmp(probe_request, HWM_name[n]) == 0) {
			probe_flag = n;
			break;
		}
	}

	method = method_inp;
	if ((n = Probe_method()) != 0) {
		return n;
	}

	if (this_method && this_sensor) {
		if (debug_flag && method != 'A')
			fprintf(stderr, "* %s found.\n", this_sensor->Name);
		return 0;
	} else {
		return -1;
	}
}


/*-------------------------
	Getting Temperatures
  -------------------------*/

#define traFahrn(x)	((x) * 1.8 + 32.0)

int getTemp(float *t1, float *t2, float *t3)
{
	int n;
	float f, t[3] = {0.0, 0.0, 0.0};

	if (this_method->Open() != 0)
		return -1;

	if (this_sensor) {
		for (n = 0; n < 3; n++) {
			if ((f = this_sensor->Temp(this_method, n)) != 0xFFFF) {
#ifdef TEMP_LIMIT
			if (f <= TEMP_HIGH || f >= TEMP_LOW)
#endif
				t[n] = f;
			}
		}
  	}
	if (this_sensor == &it87 || this_sensor == &via686) {
	/* special treatment of IT86 and VIA868 */
		f = t[0];
		t[0] = t[1];
		t[1] = t[2];
		t[2] = f;
	} else if (this_sensor == &lm85) {
	/* special treatment of LM85 */
		f = t[0];
		t[0] = t[1];
		t[1] = f;
	}

	if (num_extemp_chip > 0) {
		if ((f = smb_ExtraTemp()) != 0xFFFF)
			t[extra_tempNO] = f;
	}

	if (fahrn_flag) {
		for (n = 0; n <= 2; n++) {
			if (t[n] != 0.0)
				t[n] = traFahrn(t[n]);
		}
	}

	*t1 = t[0]; *t2 = t[1]; *t3 = t[2];

	this_method->Close();
	return 0;
}

/*--------------------
	Getting Voltages
  --------------------*/

int getVolt(float *vc0, float *vc1,\
			float *v33, float *v50p, float *v50n,\
			float *v12p, float *v12n)
{
	int n;
	float f, v[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

	if (this_method->Open() != 0)
		return -1;

	if (this_sensor) {
		for (n = 0; n < 7; n++) {
			if ((f = this_sensor->Volt(this_method, n)) != 0xFFFF)
				v[n] = f;
		}
	}
	if (this_sensor == &lm85) {
	/* special treatment of LM85 */
		f = v[0];
		v[0] = v[1];
		v[1] = f;
	}

	*vc0 = v[0], *vc1 = v[1];
	*v33 = v[2], *v50p = v[3], *v12p = v[4];
	*v12n = v[5], *v50n = v[6];

	this_method->Close();
	return 0;
}

/*----------------------
	Getting Fan Speed
  ----------------------*/

int getFanSp(int *r1, int *r2, int *r3)
{
	int n;
	int i, r[3] = {0,0,0};

	if (this_method->Open() != 0)
		return -1;

	if (this_sensor) {
		for (n = 0; n < 3; n++) {
			if ((i = this_sensor->FanRPM(this_method, n)) != 0xFFFF)
				r[n] = i;
		}
	}
	if (this_sensor == &lm85) {
	/* special treatment of LM85 */
		i = r[0];
		r[0] = r[1];
		r[1] = i;
	}
	*r1 = r[0]; *r2 = r[1]; *r3 = r[2];

	this_method->Close();
	return 0;
}
