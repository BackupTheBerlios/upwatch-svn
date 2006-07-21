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

int debug_flag = 0;

#define PCI_LIST
#define PCI_SMB_INCLUDED
#include "pci_smb.c"

int main()
{
	int hwm_base, smb_base, i, n;
	u_int chipA[256];
	int smb_baseA[256];

	printf("Listing Bus:0 PCI Configuration: ChipID\n");
	if ((n = pci_smb_prob(smb_baseA, chipA)) == 1) {
		printf("\nOnly one SMBus candidate found!  ");
		printf("ChipID=0x%08X SMBus Base(?)=0x%04X.\n\n",\
				chipA[0], smb_baseA[0]);
	} else if (n > 0) {
		printf("\n");
		printf("%d SMBus candidates found!\n", n);
		for(i = 0; i < n; ++i)
			printf("ChipID=0x%08X SMBus Base(?)=0x%04X.\n",\
				chipA[i], smb_baseA[i]);
		printf("\n");
	} else if (n == 0) {
		printf("No SMBus candidates found.\n");
	} else if (n == -1) {
		fprintf(stderr, "Cannot open IO-port, needs privilege!\n");
		exit(-1);
	}
	switch (pci_pm_smb_prob(&hwm_base, &smb_base)) {
		case VIA686HWM:
			printf("VIA686(KT133 south)'s HWM found,");
			printf(" its Base Address: 0x%04X.\n", hwm_base);
			printf("  SMBus Base Address: 0x%04X.\n", smb_base);
			break;
		case VIA686SMB:
			printf("VIA686(KT133 south) found, but HWM not available.\n");
			printf("  SMBus Base Address: 0x%04X.\n", smb_base);
			break;
		case VIA596SMB:
			printf("VIA596 found.\n");
			printf("  SMBus Base Address: 0x%04X.\n", smb_base);
			break;
		case PIIX4SMB:
			printf("IntelPIIX4(440BX south) found.\n");
			printf("  SMBus Base Address: 0x%04X.\n", smb_base);
			break;
		case SRVWSSSMB:
			printf("ServerWorks(ServerSet Chipset) found.\n");
			printf("  SMBus Base Address: 0x%04X.\n", smb_base);
			break;
		case VIA8233SMB:
			printf("VIA8233(KT266/KT333 south) found.\n");
			printf("  SMBus Base Address: 0x%04X.\n", smb_base);
			break;
		case ICH801SMB:
			printf("Intel801/810/815 (ICH/ICH2) found.\n");
			printf("  SMBus Base Address: 0x%04X.\n", smb_base);
			break;
		case AMD756SMB:
			printf("AMD756/766/768 found.\n");
			printf("  SMBus Base Address: 0x%04X.\n", smb_base);
			break;
		case NFORCESMB:
			printf("NVidia nForce found.\n");
			printf("  SMBus Base Address: 0x%04X.\n", smb_base);
			break;
		case ALI7101SMB:
			printf("ALi M1535D+ found.\n");
			printf("  SMBus Base Address: 0x%04X.\n", smb_base);
			break;
		case AMD8111SMB:
			printf("AMD8111 found.\n");
			printf("  SMBus Base Address: 0x%04X.\n", smb_base);
			break;
		case NFORCE2SMB:
			printf("NVidia nForce2 found.\n");
			printf("  SMBus Base Address: 0x%04X.\n", smb_base);
			break;
		default:
			printf("No hardware managent found.\n");
	}
	exit (0);
}
