
/* Looking for Power Management ChipSet by checking
	the PCI Configuration Register, by YRS 2001.08.

	Information on how to access SMBus is provided
	by ":p araffin.(Yoneya)", MANY THANKS!!

	Information for SMBus access and PCI chipset comes from
	the Linux lm_sensor codes: http://www.lm-sensors.nu
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef DEBUG
#define PCI_DEBUG
#include <stdio.h>
#endif

#include <unistd.h>
#ifdef NETBSD
#include <machine/pio.h>
#endif

#include "pci_pm.h"
#include "io_static.c"
#include "smbuses.h"

#ifdef LINUX	/* LINUX */
/* counter for not calling iopl() multiply */
int iopl_counter = 0;
#elif NETBSD
#else			/* FreeBSD */
/* file descripter for /dev/io */
int iofl;
#endif


char *chk_smb_chip(int chip)
{
	char *comt;

	switch (chip) {
		case VIA686HWM:
		case VIA686SMB:
			comt = "VIA82C686(KT133/A)";
			break;
		case VIA596SMB:
			comt = "VIA596";
			break;
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
		case VIA586PM:
			comt = "VIA586";
			break;
#endif
		case PIIX4SMB:
			comt = "IntelPIIX4(440BX/MX)";
			break;
		case SRVWSSSMB:
			comt = "ServerWorks(ServerSet Chipset)";
			break;
		case VIA8233SMB:
			comt = "VT8233/A/8235(KT266/KT333/KT400)";
			break;
		case VIA8235PM:
			comt = "VIA8231/8235PM";
			break;
		case ICH801SMB:
			comt = "Intel8XX(ICH/ICH2/ICH3/ICH4/ICH5)";
			break;
		case AMD756SMB:
			comt = "AMD756/766/768";
			break;
		case NFORCESMB:
			comt = "NVidia nForce";
			break;
		case ALI7101SMB:
			comt = "ALi M1535D+";
			break;
		case AMD8111SMB:
			comt = "AMD8111";
			break;
		case NFORCE2SMB:
			comt = "NVidia nForce2";
			break;
		default:
			comt = NULL;
	}
	return comt;
}

int chk_port_byte(int addr)
{
	int ret;
	if(OpenIO() == -1) return -1;
	ret = inb((u_short) addr);
	CloseIO();
	return ret;
}

int pci_pm_smb_prob(int *hwm_base, int *smb_base)
{
	u_int dat;
	u_char dev, fun;
	int ret = 0, inq_smbba = 0;

	if(OpenIO() == -1) return -1;
	*hwm_base = *smb_base = 0;
	for (dev = 0; dev < PCI_DEVM; ++dev) {
	  for (fun = 0; fun < PCI_FUNM; ++fun) {
		switch (pci_conf_read(PCI_BUSN, dev, fun, 0x00)) {
		case ID_VIA686:
			dat = pci_conf_read(PCI_BUSN, dev, fun, VIA686HWM_prob);
			if (dat && 0x00000001) {
				*hwm_base = 0xFFFE & \
					pci_conf_read(PCI_BUSN, dev, fun, VIA686HWM_base);
				ret = VIA686HWM;
			} else {
				ret = VIA686SMB;
			}
			inq_smbba = getSMBBA0;
			break;
		case ID_VIA596:
		case ID_VIA596B:
			ret = VIA596SMB;
			inq_smbba = getSMBBA0;
			break;
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
		case ID_VIA586:
			ret = VIA586PM;
			break;
#endif
		case ID_VIA8235M:
			ret = VIA8235PM;
			inq_smbba = getSMBBA0;
			break;
		case ID_PIIX4:
		case ID_PII440MX:
		case ID_EFVIC66:
			ret = PIIX4SMB;
			inq_smbba = getSMBBA0;
			break;
		case ID_SRVWSB4:
		case ID_SRVWSB5:
			ret = SRVWSSSMB;
			inq_smbba = getSMBBA0;
			break;
		case ID_VIA8233:
		case ID_VIA8233A:
		case ID_VIA8233C:
		case ID_VIA8235:
			ret = VIA8233SMB;
			inq_smbba = getSMBBA1;
			break;
		case ID_I801AA:
		case ID_I801AB:
		case ID_I801BA:
		case ID_I801CA:
		case ID_I801DB:
		case ID_I801EB:
			ret = ICH801SMB;
			inq_smbba = getSMBBA2;
			break;
		case ID_AMD756:
		case ID_AMD766:
		case ID_AMD768:
			ret = AMD756SMB;
			inq_smbba = getSMBBA3;
			break;
		case ID_NFORCE:
			ret = NFORCESMB;
			inq_smbba = getSMBBA4;
			break;
		case ID_ALI7101:
			ret = ALI7101SMB;
			inq_smbba = getSMBBA5;
			break;
		case ID_AMD8111:
			ret = AMD8111SMB;
			inq_smbba = getSMBBA6;
			break;
		case ID_NFORCE2:
			ret = NFORCE2SMB;
			inq_smbba = getSMBBA71;
			break;
		default:
		}
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
		if (ret)
			goto ending;
#else
		if (ret) {
			*smb_base = pci_conf_readw(PCI_BUSN, dev, fun, (u_char) inq_smbba);
			if (ret/10 == AMD8111SMB/10) {
				*smb_base &= 0xFFFE;
				if (ret == NFORCE2SMB) {
#ifdef PCI_DEBUG
printf("DEBUG nForce2:SMBBA1(0x%02X) --->smb_base = 0x%0X\n",
	getSMBBA71, *smb_base);
#endif
					if (!chk_smbus_io(NFORCE2SMB, *smb_base)) {
						*smb_base = pci_conf_readw(PCI_BUSN, dev, fun,
									(u_char) getSMBBA70) & 0xFFFE;
#ifdef PCI_DEBUG
printf("DEBUG nForce2:SMBBA0(0x%02X) --->smb_base = 0x%0X\n",
	getSMBBA70, *smb_base);
#endif
						if (!chk_smbus_io(NFORCE2SMB, *smb_base))
							*smb_base = 0x00;
					}
				}
			} else if (ret/10 == AMD756SMB/10) {
				*smb_base &= 0xFF00;
				if (ret == AMD756SMB)
					*smb_base += AMD_SMBOFF;
			} else if (ret == ALI7101SMB) {
				*smb_base &= 0xFFE0;
			} else {
				*smb_base &= 0xFFF0;
			}
			goto ending;
		}
#endif
	  }
	}
ending:
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
#else
	if (*smb_base < 0x100) {	/* not so confident but ... */
		ret = 0;
	}
#endif
	CloseIO();
	return ret;
}


/* Byte/Word read-out from PCI Configuration Registor.
	Here: Boundary considered.
 */

u_char pci_conf_readb(u_char bus, u_char dev, u_char fun, u_char reg)
{
	u_int dat, n;

	n = reg % 4;
	dat = pci_conf_read(bus, dev, fun, (reg & 0xFC));
	if (n == 0) {
		return (dat & 0xFF);
	} else {
		return ((dat >> (8*n)) & 0xFF);
	}
}

u_short pci_conf_readw(u_char bus, u_char dev, u_char fun, u_char reg)
{
	u_int dat, n;

	n = reg % 4;
	dat = pci_conf_read(bus, dev, fun, (reg & 0xFC));
	if (n == 0) {
		return (dat & 0xFFFF);
	} else if (n != 3) {
		return ((dat >> (8*n)) & 0xFFFF);
	} else {
		n = pci_conf_read(bus, dev, fun, (reg & 0xFC) + 4);
		return ((((dat >> (8*3)) & 0xFF) & (n << (8*1))) & 0xFFFF);
	}
}

/* PCI Configuration Registor

 31              23              15              7             0
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|1|0 0 0 0 0 0 0|  Bus Num      |Dev Num  |Func |Reg Num    |0 0|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 */

u_int pci_conf_read(u_char bus, u_char dev, u_char fun, u_char reg)
{
/* NOTE: do not check length of "bus, dev, fun, reg"!! */
/*		reg = (Reg Num) << 2  */
/*		reg & 0xFC  */
/*		fun & 0x07  */
/*		dev & 0x1F  */
/*		bus & 0xFF  */

	u_int pix = 0, dat;
	pix |= reg;
	pix |= (fun << 8);
	pix |= (dev << 11);
	pix |= (bus << 16);
	pix |= 0x80000000; 
	OUTl((unsigned int) PCI_CFIO, pix); WAIT;
	dat = inl((unsigned int) PCI_CREAD); WAIT;
	pix &= 0x7FFFFFFF; 
	OUTl((unsigned int) PCI_CFIO, pix); WAIT;
	return dat;
}
