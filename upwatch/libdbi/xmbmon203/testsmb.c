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

/*	Blacklist slave addresses

		int blacklist[]

	is defined, because in some motherboard (Abit BH6, as an example)
	SMBus hangs up if accessing to it, even in read access (not write!).
	One must make hardware reset (or switching off) to recover!
	Reference: detect.c by Takanori Watanabe.
 */

int main()
{
	int smb_base, i, j, k, n, num_smb;
	u_int chipID;
	u_int chipA[256];
	int smb_baseA[256];

	int blacklist[] = {0xD2,-1};

	if ((n = pci_smb_prob(smb_baseA, chipA)) == 1) {
		smb_base = smb_baseA[0];
		chipID = chipA[0];
		printf("Only one SMBus candidate found!  ChipID=0x%08X.\n", chipID);
	} else if (n == 0) {
		fprintf(stderr, "No SMBus candidates found!!\n");
		exit(-1);
	} else if (n > 1) {
		printf("%d SMBus candidates found!???  Checking...\n", n);
		for(i = 0; i < n; ++i)
			printf("ChipID=0x%08X SMBus Base(?)=0x%04X.\n",\
				chipA[i], smb_baseA[i]);
	} else if (n == -1) {
		fprintf(stderr, "Cannot open IO-port, needs privilege!\n");
		exit(-1);
	}

/* Big roop for smb_base candidates */
  num_smb = n;
  for (n = 0; n < num_smb; n++) {
	smb_base = smb_baseA[n];
	chipID = chipA[n];
	fprintf(stderr, "\n*** Testing SMBus base = 0x%04X ***\n", smb_base);

	smbus = &smbus_piix4;
	switch (chipID) {
	case ID_VIA686:
		fprintf(stderr, "VIA686(KT133 south) found.\n");
		break;
	case ID_VIA596:
	case ID_VIA596B:
		fprintf(stderr, "VIA596 found.\n");
		break;
	case ID_PIIX4:
	case ID_PII440MX:
	case ID_EFVIC66:
		fprintf(stderr, "IntelPIIX4(440BX south) found.\n");
		break;
	case ID_SRVWSB4:
	case ID_SRVWSB5:
		fprintf(stderr, "ServerWorks(ServerSet Chipset) found.\n");
		break;
	case ID_VIA8233:
	case ID_VIA8233A:
	case ID_VIA8233C:
	case ID_VIA8235:
		fprintf(stderr, "VIA8233/A/8235(KT266/KT333/KT400 south) found.\n");
		break;
	case ID_I801AA:
	case ID_I801AB:
	case ID_I801BA:
	case ID_I801CA:
		fprintf(stderr, "Intel801/810/815 (ICH/ICH2) found.\n");
		break;
	case ID_AMD756:
	case ID_AMD766:
	case ID_AMD768:
		smbus = &smbus_amd;
		fprintf(stderr, "AMD756/766/768 found.\n");
		break;
	case ID_NFORCE:
		smbus = &smbus_amd;
		fprintf(stderr, "NVidia nFORCE found.\n");
		break;
	case ID_ALI7101:
		smbus = &smbus_ali;
		fprintf(stderr, "ALi M1535D+ found.\n");
		break;
	case ID_AMD8111:
		smbus = &smbus_amd8;
		fprintf(stderr, "AMD8111 found.\n");
		break;
	case ID_NFORCE2:
		smbus = &smbus_amd8;
		fprintf(stderr, "NVidia nForce2 found.\n");
		break;
	default:
		fprintf(stderr, "No known SMBus(I2C) chip found.\n");
		goto exit;
	}

	if(OpenIO() == -1) return -1;

/* Testing SMBus devices */
	for (i = 0; i < 255; i += 2) {
	  for (j = 0; blacklist[j] != -1; j++) {
	 	if (i != blacklist[j] && (k = smbus->ReadB(smb_base, i, 0x00)) != -1) {
			printf(" addr:0x%02X ---> byte of Reg0 =0x%02X\n", i, k);
		}
	  }
	}
	
	CloseIO();
exit:

  } /* endo of Big roop for smb_base candidates */
  exit (0);

}
