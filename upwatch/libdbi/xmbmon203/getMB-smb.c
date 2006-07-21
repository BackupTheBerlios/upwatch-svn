
/* SMBus method for accessing Hardware Monitor Chip */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)

/* Using SMBus ioctl, need "xxxpm" driver !! */
#include "getMB-smb_ioctl.c"

#else		/* SMBus direct access routines */

#include "methods.h"
#include "smbuses.h"

/* smbus io routine, global */
extern SMBUS_IO *smbus;

/* SMBus Base Address, global */
int smb_base	= -1;
int smb_slave	= LM_ADDR;
int smb_wbtemp1_flag = 1;	/* = 0 if enable */
int smb_wbtemp2_flag = 1;	/* = 0 if eaable */
int smb_wbtemp1	= WBtemp1_ADDR;
int smb_wbtemp2	= WBtemp2_ADDR;
int smb_wbt1reg	= 0x00;
int smb_wbt2reg	= 0x00;

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "io_static.c"


static int ReadByte(int addr)
{
	int ret;
	ret = smbus->ReadB(smb_base, smb_slave, addr);
	if (ret == -1) {
/*	perror("SMBus ReadB");
		exit(-1); */
		ret = 0xFF;
	}
	return (ret & 0xFF);
}

static void WriteByte(int addr, int value)
{
	if(smbus->WriteB(smb_base, smb_slave, addr, value) == -1) {
/*		perror("SMBus WriteB");
		exit(-1); */
	}
}

static int ReadWord(int addr)
{
	int ret;
	ret = smbus->ReadW(smb_base, smb_slave, addr);
	if (ret == -1) {
/*	perror("SMBus ReadW");
		exit(-1); */
		ret = 0xFFFF;
	}
	return (ret & 0xFFFF);
}

static void WriteWord(int addr, int value)
{
	if(smbus->WriteW(smb_base, smb_slave, addr, value) == -1) {
		perror("SMBus WriteW");
		exit(-1);
	}
}

static int ReadTemp1()
{
	int ret;
	ret = smbus->ReadW(smb_base, smb_wbtemp1, smb_wbt1reg);
	if (ret == -1) {
/*		perror("SMBus ReadW");
		exit(-1); */
		ret = 0xFFFF;
	}
	return (ret & 0xFFFF);
}
static int ReadTemp2()
{
	int ret;
	ret = smbus->ReadW(smb_base, smb_wbtemp2, smb_wbt2reg);
	if (ret == -1) {
/*		perror("SMBus ReadW");
		exit(-1); */
		ret = 0xFFFF;
	}
	return (ret & 0xFFFF);
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
