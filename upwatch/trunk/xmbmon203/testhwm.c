#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
char * smb_devfile = "/dev/smb0";
#define NO_INCLUDE_SMBIO
#include "getMB-smb_ioctl.c"
#undef NO_INCLUDE_SMBIO
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>

#undef PCI_LIST
#define PCI_SMB_INCLUDED
#include "pci_smb.c"
#include "smbuses.h"

int debug_flag = 0;


/* smbus io routines, global */
extern SMBUS_IO smbus_piix4, smbus_amd, smbus_ali, smbus_amd8;

static SMBUS_IO *smbus;

#define LM_ADDR	0x5A

#define ReadByte(x)     (smbus->ReadB(smb_base, LM_ADDR, (x)))
#define WriteByte(x, y) (smbus->WriteB(smb_base, LM_ADDR, (x), (y)))


int main()
{
	int i, n;
	int viahwm_base, smb_base;
	int Reg47, Reg48, Reg49;
	char *smb = "";

	n = pci_pm_smb_prob(&viahwm_base, &smb_base);

	if (n <= 0) {
		fprintf(stderr, "No SMBus candidates found!!\n");
		exit(-1);
	} else if (n > 0) {
		switch (n) {
			case VIA686SMB:
				smb = "VIA686SMB";
				break;
			case VIA686HWM:
				smb = "VIA686HWM";
				break;
			case VIA596SMB:
				smb = "VIA596SMB";
				break;
			case PIIX4SMB:
				smb = "PIIX4SMB";
				break;
			case SRVWSSSMB:
				smb = "SRVWSSSMB";
				break;
			case VIA8233SMB:
				smb = "VIA8233SMB";
				break;
			case ICH801SMB:
				smb = "ICH801SMB";
				break;
			case AMD756SMB:
				smb = "AMD756SMB";
				break;
			case NFORCESMB:
				smb = "NFORCESMB";
				break;
			case ALI7101SMB:
				smb = "ALI7101SMB";
				break;
			case AMD8111SMB:
				smb = "AMD8111SMB";
				break;
			case NFORCE2SMB:
				smb = "NFORCE2SMB";
				break;
		}
		printf(" *** SMBus %s found! --- SMBus Base=0x%04X.\n", smb, smb_base);
	}
	if (n/10 == AMD756SMB/10)
		smbus = &smbus_amd;
	else if (n/10 == ALI7101SMB/10)
		smbus = &smbus_ali;
	else if (n/10 == AMD8111SMB/10)
		smbus = &smbus_amd8;
	else
		smbus = &smbus_piix4;

/* Big roop for smb_base candidates */
	fprintf(stderr, "\n*** Testing SMBus base = 0x%04X ***\n", smb_base);

	if(OpenIO() == -1) return -1;

	Reg47 = ReadByte(0x47);
	i = Reg47;
	printf("  CheckingFanDV(CR47)  = %02X\n", i);

	Reg48 = ReadByte(0x48);
	i = Reg48&0x7F;
	printf("  MainSMBusADDR(CR48)  = %02X\n", 2*i);

	Reg49 = ReadByte(0x49);
	i = Reg49;
	printf("  DeviceID(CR49&0xFE)  = %02X\n", i&0xFE);

	if (Reg47 == -1 && Reg48 == -1 && Reg49 == -1) {
		fprintf(stderr, "No Hardware Monitor found on this SMBus base!\n");
		goto exit;
	}

	WriteByte(0x4E, 0x00);

	i = ReadByte(0x4F);
	printf("  VenderID(CR4E=0,CR4F)= %02X\n", i);

	i = ReadByte(0x58);
	printf("  ChipID(CR58)         = %02X\n", i);

	i = ReadByte(0x27);
	printf("   Temp0(CR27)         = %02X\n", i);

	i = ReadByte(0x20);
	printf("   Vcore(CR20)         = %02X\n", i);

	i = ReadByte(0x22);
	printf("   V3.3 (CR22)         = %02X\n", i);

	i = ReadByte(0x23);
	printf("   V5.0 (CR23)         = %02X\n", i);

	i = ReadByte(0x24);
	printf("   V12.0(CR24)         = %02X\n", i);

#ifdef FAN_DIV_CHK
/*
	NOTE:
	I don't know why, but if system function "sleep()"
	is called while "/dev/io" is being opened,
	"xmbmon" crashes (it goes into infinite loop
	somewhere in kernel or shared library).
	So, it is important to close it when calling sleep()!
*/
	{
	int reg, i1, i2;
	CloseIO();

	printf("\nFan Devisor(CR47) Check:\n");

	if(OpenIO() == -1) return -1;

	reg = (Reg47 & 0x0F) | 0x00;	/* 0000 : divisor 1 1 */
	printf(" write(CR47,0?:11) --->");
	WriteByte(0x47, reg);
	CloseIO(); sleep(2); if(OpenIO() == -1) return -1;
	i = ReadByte(0x28); i1 = ReadByte(0x29); i2 = ReadByte(0x2A);
	printf(" CR28,CR29,CR2A = %02X,%02X,%02X\n", i, i1, i2);

	reg = (Reg47 & 0x0F) | 0x10;	/* 0001 : divisor 2 1 */
	printf(" write(CR47,1?:21) --->");
	WriteByte(0x47, reg);
	CloseIO(); sleep(2); if(OpenIO() == -1) return -1;
	i = ReadByte(0x28); i1 = ReadByte(0x29); i2 = ReadByte(0x2A);
	printf(" CR28,CR29,CR2A = %02X,%02X,%02X\n", i, i1, i2);

	reg = (Reg47 & 0x0F) | 0x20;	/* 0010 : divisor 4 1 */
	printf(" write(CR47,2?:41) --->");
	WriteByte(0x47, reg);
	CloseIO(); sleep(2); if(OpenIO() == -1) return -1;
	i = ReadByte(0x28); i1 = ReadByte(0x29); i2 = ReadByte(0x2A);
	printf(" CR28,CR29,CR2A = %02X,%02X,%02X\n", i, i1, i2);

	reg = (Reg47 & 0x0F) | 0x30;	/* 0011 : divisor 8 1 */
	printf(" write(CR47,3?:81) --->");
	WriteByte(0x47, reg);
	CloseIO(); sleep(2); if(OpenIO() == -1) return -1;
	i = ReadByte(0x28); i1 = ReadByte(0x29); i2 = ReadByte(0x2A);
	printf(" CR28,CR29,CR2A = %02X,%02X,%02X\n", i, i1, i2);

	reg = (Reg47 & 0x0F) | 0x40;	/* 0100 : divisor 1 2 */
	printf(" write(CR47,4?:12) --->");
	WriteByte(0x47, reg);
	CloseIO(); sleep(2); if(OpenIO() == -1) return -1;
	i = ReadByte(0x28); i1 = ReadByte(0x29); i2 = ReadByte(0x2A);
	printf(" CR28,CR29,CR2A = %02X,%02X,%02X\n", i, i1, i2);

	reg = (Reg47 & 0x0F) | 0x80;	/* 1000 : divisor 1 4 */
	printf(" write(CR47,8?:14) --->");
	WriteByte(0x47, reg);
	CloseIO(); sleep(2); if(OpenIO() == -1) return -1;
	i = ReadByte(0x28); i1 = ReadByte(0x29); i2 = ReadByte(0x2A);
	printf(" CR28,CR29,CR2A = %02X,%02X,%02X\n", i, i1, i2);

	reg = (Reg47 & 0x0F) | 0xC0;	/* 1100 : divisor 1 8 */
	printf(" write(CR47,C?:18) --->");
	WriteByte(0x47, reg);
	CloseIO(); sleep(2); if(OpenIO() == -1) return -1;
	i = ReadByte(0x28); i1 = ReadByte(0x29); i2 = ReadByte(0x2A);
	printf(" CR28,CR29,CR2A = %02X,%02X,%02X\n", i, i1, i2);

	WriteByte(0x47, Reg47);
	}
#endif

exit:
	CloseIO();

 	return 0; 

}
