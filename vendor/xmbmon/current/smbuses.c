/* setting SMBus IO routines for various chips */  

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pci_pm.h"
#include "smbuses.h"

#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
extern int smbioctl_readB(int, int, int);
#else			/* using own SMBus IO routines */
/* smbus io routines, global */
SMBUS_IO *smbus;
extern SMBUS_IO smbus_piix4;
extern SMBUS_IO smbus_amd;
extern SMBUS_IO smbus_ali;
extern SMBUS_IO smbus_amd8;
#endif

int set_smbus_io(int *viahwm_base, int *smb_base)
{
	int n;

	n = pci_pm_smb_prob(viahwm_base, smb_base);
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
	return n;
#else			/* using own SMBus IO routines */
	if (n <= 0)
		return n;

	if (n/10 == AMD756SMB/10)
		smbus = &smbus_amd;
	else if (n/10 == ALI1535SMB/10)
		smbus = &smbus_ali;
	else if (n/10 == AMD8111SMB/10)
		smbus = &smbus_amd8;
	else
		smbus = &smbus_piix4;

	return n;
#endif
}

/*	Blacklist slave addresses

		int blacklist[]

	is defined, because in some motherboard (Abit BH6, as an example)
	SMBus hangs up if accessing to it, even in read access (not write!).
	One must make hardware reset (or switching off) to recover!
	Reference: detect.c by Takanori Watanabe.
 */

int chk_smbus_io(int smb_chipset, int smb_base)
{
	int n, i, j;
	int blacklist[] = {0xD2,-1};
	int (*readB)(int, int, int);

	if (smb_chipset <= 0)
		return 0;

#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
	readB = smbioctl_readB;
#else
	if (smb_chipset/10 == AMD756SMB/10)
		smbus = &smbus_amd;
	else if (smb_chipset/10 == ALI1535SMB/10)
		smbus = &smbus_ali;
	else if (smb_chipset/10 == AMD8111SMB/10)
		smbus = &smbus_amd8;
	else
		smbus = &smbus_piix4;

	readB = smbus->ReadB;
#endif

	n = 0;
	for (i = 0; i < 255; i += 2) {
	  for (j = 0; blacklist[j] != -1; j++) {
	 	if (i != blacklist[j] && readB(smb_base, i, 0x00) != -1)
			n++;
	  }
	}
	return n;
}
