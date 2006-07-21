

/* Looking for Power Management ChipSet by checking
	the PCI Configuration Register, by YRS 2001.08.
	Information on how to access SMBus is provided
	by ":p araffin.(Yoneya)", MANY THANKS!!

	Many information about PCI chipsets comes from
	the lm_sensors codes, MANY THANKS!!
 */

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "io_cpu.h"

#ifndef NULL
#define NULL 0x00
#endif

/* For getting the PCI Configuration */
#define PCI_BUSN	0x0000	/* assume PCI Bus number = 0x00 for chipset */
#define PCI_REGM	64
#define PCI_DEVM	32
#define PCI_FUNM	8

#define PCI_CFIO	0x0CF8
#define PCI_CREAD	0x0CFC

/* The offset of the SMBus base address */
#define getSMBBA0	0x90	/* PIIX4, VIA596,686 */
#define getSMBBA1	0xD0	/* VT8233/A/C */
#define getSMBBA2	0x20	/* Intel801_ICH's */
#define getSMBBA3	0x58	/* AMD756,766,768 */
#define getSMBBA4	0x14	/* NVidia nForce */
#define getSMBBA5	0xE2	/* ALI7101 */
#define getSMBBA6	0x10	/* AMD8111 */
#define getSMBBA70	0x50	/* NVidia nForce2, bus0 */
#define getSMBBA71	0x54	/* NVidia nForce2, bus1 */

#define AMD_SMBOFF	0xE0	/* for AMD756, extra offset */

/* The PM chipset's chipID and vendorID (0x[chipID][vendorID]) */
#define ID_VIA586	0x30401106
#define ID_VIA596	0x30501106
#define ID_VIA596B	0x30511106
#define ID_VIA686	0x30571106
#define ID_VIA8233	0x30741106
#define ID_VIA8233A	0x31471106
#define ID_VIA8233C	0x31091106
#define ID_VIA8235	0x31771106
#define ID_VIA8235M	0x82351106
#define ID_PIIX4	0x71138086
#define ID_PII440MX	0x719B8086
#define ID_SRVWSB4	0x02001166
#define ID_SRVWSB5	0x02011166
#define ID_EFVIC66	0x94631055
#define ID_I801AA	0x24138086
#define ID_I801AB	0x24238086
#define ID_I801BA	0x24438086
#define ID_I801CA	0x24838086
#define ID_I801DB	0x24C38086
#define ID_I801EB	0x24D38086
#define ID_AMD756	0x740B1022
#define ID_AMD766	0x74131022
#define ID_AMD768	0x74431022
#define ID_AMD8111	0x746A1022
#define ID_NFORCE	0x01B410DE
#define ID_NFORCE2	0x006410DE
#define ID_ALI7101	0x710110B9

#define VIA686HWM_prob	0x74
#define VIA686HWM_base	0x70

/* Here ID's for the class of SMBus used inside mbmon/xmbmon */
#define VIA686SMB	01
#define VIA686HWM	02
#define VIA596SMB	11
#define VIA8235PM	12
#define VIA586PM	13
#define PIIX4SMB	21
#define SRVWSSSMB	22
#define VIA8233SMB	31
#define ICH801SMB	41
#define AMD756SMB	51
#define NFORCESMB	52
#define ALI7101SMB	61
#define AMD8111SMB	71
#define NFORCE2SMB	72


int chk_port_byte(int);
char *chk_smb_chip(int);
int pci_pm_smb_prob(int *, int *);
int pci_smb_prob(int [], u_int []);
u_char pci_conf_readb(u_char, u_char, u_char, u_char);
u_short pci_conf_readw(u_char, u_char, u_char, u_char);
u_int pci_conf_read(u_char, u_char, u_char, u_char);
