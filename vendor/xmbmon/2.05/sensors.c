/* functions used commonly in each hardware monitor module */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef DEBUG
#define SMB_DEBUG
#endif

#include <stdio.h>
#define INCLUDE_HWM_MODULE
#include "sensors.h"
#undef INCLUDE_HWM_MODULE

/* external (global) data */
extern int debug_flag;
extern int smb_base;
extern int smb_slave;
extern int smb_wbtemp1, smb_wbtemp2;
extern LM_METHODS method_smb;

/* SMBus Slave Address Candidates, global */
int numSMBSlave	= 0;
int canSMBSlave[128];


int chkReg_Probe(int slave, char *comment, int Reg[], LM_METHODS *method)
{
	int i, n, r, ret = 0;

	if (slave > 0 && debug_flag > 1)
		fprintf(stderr, "Set SMBus slave address: 0x%02X\n", slave);
	if (debug_flag > 1)
		fprintf(stderr, "%s", comment);
	for (i = 0; (r = Reg[i]) != -1; i++) {
		n = method->Read(r);
		if (n != 0xFF)
			++ret;
		if (debug_flag > 1) {
			if ((i + 1) % 4 == 0)
				fprintf(stderr, "  CR%02X:0x%02X\n", r, n);
			else
				fprintf(stderr, "  CR%02X:0x%02X,", r, n);
		}
	}
	if (debug_flag > 1) {
		if(( i & 3 ) != 0 )
			fprintf( stderr, "\n" ) ;
	}
#if 0
	if (debug_flag > 1)
		fprintf(stderr, "\n");
#endif

	return ret;
}

int chkReg_Probe_strict(int Reg[], LM_METHODS *method)
{
	int i, r;

	for (i = 0; (r = Reg[i]) != -1; i++) {
		if (method->Read(r) == 0xFF)
			return 0;
	}
	return 1;
}


/* Register checked for scanning smbus */
static int smb_scanReg[] = {
	0x00, 0x01, 0x20, 0x40, 0x48,
	-1 };

/* Blacklist slave address, do not access */
static int blacklist[] = {
	0xD2,
	-1 };

int scan_smbus(int addr_start, int addr_end, int result[])
{
	int i, j, n, r, save, reg, ret = 0;
	LM_METHODS *method = &method_smb;

#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
#else
	if (smb_base <= 0)
		return ret;
#endif

	addr_start = 2 * (addr_start/2);
	if (addr_start <= 0)
		addr_start = 2;
	if (addr_end > 0xFE)
		addr_end = 0xFE;

	save = smb_slave;
	method->Open();
	for (j = addr_start, ret = 0; j <= addr_end; j += 2) {
		smb_slave = j;
		for (i = 0; blacklist[i] != -1; i++)
			if (j == blacklist[i])
				goto skip;
#ifdef SMB_DEBUG
if (debug_flag > 1)
fprintf(stderr, "SLAVE::0x%02X\n", j);
#endif
		for (i = 0, n = 0; (r = smb_scanReg[i]) != -1; i++) {
			/* != 0xFF is not enough; need to check more, but ... */
			if (((reg = method->Read(r)) & 0xFF) != 0xFF)
				++n;
#ifdef SMB_DEBUG
if (debug_flag > 1)
fprintf(stderr, "  0x%02X:0x%02X\n", r, reg);
#endif
		}
		if (n) {
			result[ret++] = j;
			if (debug_flag > 1)
				fprintf(stderr,
			" SMBus slave 0x%02X(0x%02X) found...\n", j, j/2);
		}
skip:
		continue;
	}
	method->Close();
	smb_slave = save;
	return ret;
}

int find_smb_dev(void)
{
	return (numSMBSlave = scan_smbus(0x00, 0xFE, canSMBSlave));
}

int get_smb_slave(int start, int end)
{
	int i;
	for (i = 0; i < numSMBSlave; i++) {
		if (start <= canSMBSlave[i] && canSMBSlave[i] <= end)
			break;
	}
	if (i >= numSMBSlave)
		return 0;
	else
		return canSMBSlave[i];
}

void kill_smb_slave(int slave)
{
	int i;
	for (i = 0; i < numSMBSlave; i++) {
		if (slave == canSMBSlave[i]) {
			canSMBSlave[i] = 0xFF;
			break;
		}
	}
}

int set_smb_Extemp(int start, int end, int *temp1, int *temp2)
{
    int i, temp1_flag = 1, temp2_flag = 1;    /* disable! */

    for (i = 0; i < numSMBSlave; i++) {
        if (start <= canSMBSlave[i] && canSMBSlave[i] <= end) {
            if (temp1_flag) {
                temp1_flag = 0; /* enabled! */
                *temp1 = canSMBSlave[i];
            } else if (temp2_flag) {
                temp2_flag = 0; /* enabled! */
                *temp2 = canSMBSlave[i];
            }
        }
    }
	return ((temp1_flag << 1) + temp2_flag);
}

void info_Extemp(LM_METHODS *method, int temp1, int temp2)
{
    if (!temp1) {
        if (debug_flag > 1) {
			if (method == &method_smb)
	            fprintf(stderr,"* Temp1 exists at 0x%02X,", smb_wbtemp1);
			else
	            fprintf(stderr,"* Temp1 exists at Bank 1,");
    	}
    } else {
        if (debug_flag > 1)
            fprintf(stderr,"* NO Temp1,");
    }
    if (!temp2) {
        if (debug_flag > 1) {
			if (method == &method_smb)
        	    fprintf(stderr," Temp2 exists at 0x%02X.\n", smb_wbtemp2);
			else
	            fprintf(stderr," Temp2 exists at Bank 2.\n");
    	}
    } else {
        if (debug_flag > 1)
            fprintf(stderr," NO Temp2.\n");
    }
}
