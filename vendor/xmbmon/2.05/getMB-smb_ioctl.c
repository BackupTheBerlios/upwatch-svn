#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SMBUS
/* assume SMBus ioctl support, only for FreeBSD */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <machine/smb.h>

#include "methods.h"

/* SMBus Base Address, global */
int smb_base	= 2002;		/* arbitrary positive number */
int smb_slave	= LM_ADDR;
int smb_wbtemp1_flag = 1;	/* = 0 if enable */
int smb_wbtemp2_flag = 1;	/* = 0 if eaable */
int smb_wbtemp1	= WBtemp1_ADDR;
int smb_wbtemp2	= WBtemp2_ADDR;
int smb_wbt1reg	= 0x00;
int smb_wbt2reg	= 0x00;

/* SMBus device file name, global */
extern char *smb_devfile;

int smbioctl_readB(int, int);
void smbioctl_writeB(int, int, int);
int smbioctl_readW(int, int);
void smbioctl_writeW(int, int, int);

static int iosmb;
static char buf[128];

#ifndef NO_INCLUDE_SMBIO

static int OpenIO()
{
	char byte;
	struct smbcmd cmd;
	cmd.slave = smb_slave;
	cmd.data.byte_ptr = &byte;

	if ((iosmb = open(smb_devfile, 000)) < 0) {
		strcpy(buf, "ioctl(");
		strcat(buf, smb_devfile + 5);
		strcat(buf, ":open)");
		perror(buf);
		return -1;
	}
/* command Reg:0x47 is not universal for all hardware monitor chips */
#if 0
	cmd.cmd = 0x47;
	if (ioctl(iosmb, SMB_READB, &cmd) == -1) {
		strcpy(buf, "ioctl(");
		strcat(buf, smb_devfile + 5);
		strcat(buf, ":read Reg.0x47)");
		perror(buf);
		return -1;
	}
#endif
	return 0;
}

static void CloseIO()
{
	close(iosmb);
}

static int ReadByte(int addr)
{
	return smbioctl_readB(smb_slave, addr);
}

static void WriteByte(int addr, int value)
{
	smbioctl_writeB(smb_slave, addr, value);
}

static int ReadWord(int addr)
{
	return smbioctl_readW(smb_slave, addr);
}

static void WriteWord(int addr, int value)
{
	smbioctl_writeW(smb_slave, addr, value);
}

static int ReadTemp1()
{
	return smbioctl_readW(smb_wbtemp1, smb_wbt1reg);
}

static int ReadTemp2()
{
	return smbioctl_readW(smb_wbtemp2, smb_wbt2reg);
}

struct lm_methods method_smb = {
  OpenIO,
  CloseIO,
  ReadByte,
  WriteByte,
  ReadWord,
  WriteWord,
  ReadTemp1,
  ReadTemp2
};

#endif


/* From here global routines using smb_ioctl */

int smbioctl_readB(int slave, int addr)
{
	struct smbcmd cmd;
	char ret;
	cmd.slave = slave;
	cmd.cmd = addr;
	cmd.data.byte_ptr = &ret;
	if (ioctl(iosmb, SMB_READB, &cmd) == -1) {
/*		strcpy(buf, "ioctl(");
		strcat(buf, smb_devfile + 5);
		strcat(buf, ":readbyte)");
		perror(buf);
		exit(-1); */
		ret = 0xFF;
	}
	return (ret & 0xFF);
}

void smbioctl_writeB(int slave, int addr, int value)
{
	struct smbcmd cmd;
	cmd.slave = slave;
	cmd.cmd = addr;
	cmd.data.byte = value;
	if (ioctl(iosmb, SMB_WRITEB, &cmd) == -1) {
		strcpy(buf, "ioctl(");
		strcat(buf, smb_devfile + 5);
		strcat(buf, ":writebyte)");
		perror(buf);
		exit(-1);
	}
}

int smbioctl_readW(int smb_slave, int addr)
{
	struct smbcmd cmd;
	short ret;
	cmd.slave = smb_slave;
	cmd.cmd = addr;
	cmd.data.word_ptr = &ret;
	if (ioctl(iosmb, SMB_READW, &cmd) == -1) {
/*		strcpy(buf, "ioctl(");
		strcat(buf, smb_devfile + 5);
		strcat(buf, ":readword)");
		perror(buf);
		exit(-1); */
		ret = 0xFFFF;
	}
	return (ret & 0xFFFF);
}

void smbioctl_writeW(int slave, int addr, int value)
{
	struct smbcmd cmd;
	cmd.slave = slave;
	cmd.cmd = addr;
	cmd.data.word = value;
	if (ioctl(iosmb, SMB_WRITEW, &cmd) == -1) {
		strcpy(buf, "ioctl(");
		strcat(buf, smb_devfile + 5);
		strcat(buf, ":writeword)");
		perror(buf);
		exit(-1);
	}
}


#endif
