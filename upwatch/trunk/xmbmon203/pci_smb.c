
/* Looking for Power Management ChipSet by checking
	the PCI Configuration Register, by YRS 2001.08.

	Information on how to access SMBus is provided
	by ":p araffin.(Yoneya)", MANY THANKS!!

	Information for SMBus access and PCI chipset comes from
	the Linux lm_sensor codes: http://www.lm-sensors.nu
 */

#include <unistd.h>
#ifdef NETBSD
#include <machine/pio.h>
#endif

#include "pci_pm.h"
#include "io_static.c"

#ifndef PCI_SMB_INCLUDED
#ifdef LINUX	/* LINUX */
/* counter for not calling iopl() multiply */
int iopl_counter = 0;
#elif NETBSD
#else			/* FreeBSD */
/* file descripter for /dev/io */
int iofl;
#endif
#endif

int canSMBUS_base[] = {
	getSMBBA0,
	getSMBBA1,
	getSMBBA2,
	getSMBBA3,
	getSMBBA4,
	getSMBBA5,
	getSMBBA6,
	getSMBBA70,
	getSMBBA71,
	0 };


int pci_smb_prob(int smb_base[], u_int chip_id[])
{
	u_int chip, dat, addr;
	u_char dev, fun;
	int i, n = 0;

	if(OpenIO() == -1) return -1;
	for (dev = 0; dev < PCI_DEVM; ++dev) {
	  for (fun = 0; fun < PCI_FUNM; ++fun) {
		chip = pci_conf_read(PCI_BUSN, dev, fun, 0x00);
		if (chip != 0xFFFFFFFF) {
		  for (i = 0; (addr = canSMBUS_base[i]) != 0; i++) {
			dat = pci_conf_readw(PCI_BUSN, dev, fun, (u_char) addr);
#ifdef PCI_LIST
	if (i == 0) {
		printf("bus=0x00:dev=0x%02X:fun=0x%02X ---> ", dev, fun);
		printf("chip=0x%08X [0x%02X] SMBase=0x%08X\n", chip, addr, dat);
	} else {
		printf("                                                ");
		printf("[0x%02X] SMBase=0x%08X\n", addr, dat);
	}
#endif
			if (dat != 0x0 && dat != 0xFFFFFFFF \
				&& (dat & 0x0F) == 1 && (dat & 0xFFF00000) == 0) {
				chip_id[n] = chip;
				if (chip == ID_AMD756 || chip == ID_AMD766 )
					smb_base[n] = (dat & 0xFF00) + AMD_SMBOFF;
				else
					smb_base[n] = (dat & 0xFFF0);
				if (inb(smb_base[n]) != 0xFF)
					++n;
			}
		  }
		}
	  }
	}
	CloseIO();
	return n;
}
