
/* SMB handling routines for AMD8111, by YRS 2003.04.
	Information on how to access SMBus is provided
	by "Alex van Kaam", MANY THANKS!!

	Common to AMD8111 and NVidia nForce2


	SMBus IO method:

	smb_base  : Base Address
	addr      : Slave Device Address
	cmd       : Command Register 

	The following note does not apply to NVidia nForce2!
	Uses always the lowest bit 1, and read/write is distinguished
	by the control command number.

------------------------------------------------------------------------
	Note that SMBus Slave Device Address is totall 1byte data,
	whose upper 7bits is for address and the lowest bit for read (=1)
	and for write (=0).
	The input "addr" in the following routines is this 1byte data,
	where the lowest bit can be either 0 or 1.

            7             0
           +-+-+-+-+-+-+-+-+
           | Slave addr. |f|    f = 0 for write, =1 for read.
           +-+-+-+-+-+-+-+-+

	Do not confuse "Slave address" which is "addr(here)"/2!
------------------------------------------------------------------------

 */

#ifdef SMB_DEBUG
extern int debug_flag;
#include <stdio.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "smbuses.h"
#include "io_cpu.h"

#define LOOP_COUNT	0x2000
#define LOOP_WAIT	0x20

/* command address */
#define SMBGLENB 	0x0 /* SMBus global enable */
#define SMBGLSTS	0x1 /* SMBus global status */
#define SMBHADDR	0x2 /* host address register */
#define SMBHCMD     0x3 /* command register for host controller */
#define SMBHDATA0	0x4 /* data 0 register for host controller */
#define SMBHDATA1	0x5 /* data 1 register for host controller */

/* status flag */
#define SMBHSTS_OKOK	0x00	/* transaction successful */
#define SMBHSTS_NOACK	0x10	/* not acknowledged (no device) */
#define SMBHSTS_DEVERR	0x11	/* device error */
#define SMBHSTS_TIMEOUT	0x18	/* timeout */
#define SMBHSTS_PROTERR	0x19	/* protocol error */
#define SMBHSTS_BUSY	0x1A	/* device busy or arbitration lost */
#define SMBHSTS_PECERR	0x1F	/* PEC(Packet Error Checking) error */
#define SMBHSTS_ALLERR	0x1F	/* all bits of error above */
#define SMBHSTS_ALRM	0x40	/* alarm message received */
#define SMBHSTS_DONE	0x80	/* command completed */

/* control command number */
#define SMBHCTRL_BYTEW	0x06	/* write byte */
#define SMBHCTRL_BYTER	0x07	/* read byte */
#define SMBHCTRL_WORDW	0x08	/* write word */
#define SMBHCTRL_WORDR	0x09	/* read word */


static int readbyte(int smb_base, int addr, int cmd)
{
	u_char dat = 0, saddr = 2*(addr/2) + 1;
	int i;

top:
	OUTb((u_short) (smb_base + SMBHADDR), (u_char) saddr); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBGLENB), SMBHCTRL_BYTER); WAIT; WAIT;

	for (i = 0; i < LOOP_WAIT; ++i)
		WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) (smb_base + SMBGLSTS)); WAIT; WAIT;
#ifdef SMB_DEBUG
fprintf(stderr, "   Readbyte: flag = 0x%02X, loop#:%04d\n", dat, i);
#endif
		if (dat == SMBHSTS_BUSY || dat == SMBHSTS_TIMEOUT)
			goto top;
		if (dat & (SMBHSTS_DONE | SMBHSTS_ALLERR))
			break;
	}
#ifdef SMB_DEBUG
if(i != LOOP_COUNT)
fprintf(stderr,
"   fx:Readbyte: addr=0x%02X, cmd=0x%02X flag = 0x%02X, loop#:%04d\n",
addr, cmd, dat, i);
#endif
	if (dat == SMBHSTS_DONE) {
		dat = INb((u_short) (smb_base + SMBHDATA0)); WAIT; WAIT;
#ifdef SMB_DEBUG
fprintf(stderr, "   fy:Readbyte: addr=0x%02X, cmd=0x%02X, dat = 0x%02X\n",
				addr, cmd, dat);
#endif
		return (dat & 0xFF);
	} else
		return -1;
}

static int readword(int smb_base, int addr, int cmd)
{
	u_char dat = 0, saddr = 2*(addr/2) + 1;
	int i;

top:
	OUTb((u_short) (smb_base + SMBHADDR), (u_char) saddr); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBGLENB), SMBHCTRL_WORDR); WAIT; WAIT;

	for (i = 0; i < LOOP_WAIT; ++i)
		WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) (smb_base + SMBGLSTS)); WAIT; WAIT;
		if (dat == SMBHSTS_BUSY || dat == SMBHSTS_TIMEOUT)
			goto top;
		if (dat & (SMBHSTS_DONE | SMBHSTS_ALLERR))
			break;
	}
#ifdef SMB_DEBUG
if(i != LOOP_COUNT)
fprintf(stderr,
"   fx:Readword: addr=0x%02X, cmd=0x%02X flag = 0x%02X, loop#:%04d\n",
addr, cmd, dat, i);
#endif
	if (dat == SMBHSTS_DONE) {
		i = INb((u_short) (smb_base + SMBHDATA1)); WAIT; WAIT;
		dat = INb((u_short) (smb_base + SMBHDATA0)); WAIT; WAIT;
#ifdef SMB_DEBUG
fprintf(stderr, "   fy:Readword: addr=0x%02X, cmd=0x%02X, dat = 0x%02X\n",
				addr, cmd, dat);
#endif
		return (((i << 8) + dat) & 0xFFFF);
	} else
		return -1;
}

static int writebyte(int smb_base, int addr, int cmd, int value)
{
	u_char dat = 0, saddr = 2*(addr/2) + 1;
	int i;

top:
	OUTb((u_short) (smb_base + SMBHADDR), (u_char) saddr); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHDATA0), (u_char) value); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBGLENB), SMBHCTRL_BYTEW); WAIT; WAIT;

	for (i = 0; i < LOOP_WAIT; ++i)
		WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) (smb_base + SMBGLSTS)); WAIT; WAIT;
#ifdef SMB_DEBUG
fprintf(stderr, "   Writebyte: flag = 0x%02X, loop#:%04d\n", dat, i);
#endif
		if (dat == SMBHSTS_BUSY || dat == SMBHSTS_TIMEOUT)
			goto top;
		if (dat & (SMBHSTS_DONE | SMBHSTS_ALLERR))
			break;
	}
#ifdef SMB_DEBUG
if(i != LOOP_COUNT)
fprintf(stderr,
"   fx:Writebyte: addr=0x%02X, cmd=0x%02X flag = 0x%02X, loop#:%04d\n",
addr, cmd, dat, i);
#endif
	if (dat == SMBHSTS_DONE)
		return 0;
	else
		return -1;
}

static int writeword(int smb_base, int addr, int cmd, int value)
{
	u_char dat = 0, saddr = 2*(addr/2) + 1;
	int i;

top:
	OUTb((u_short) (smb_base + SMBHADDR), (u_char) saddr); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHDATA0), (u_char) (value & 0xFF)); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHDATA1), (u_char) (value >> 8)); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBGLENB), SMBHCTRL_WORDW); WAIT; WAIT;

	for (i = 0; i < LOOP_WAIT; ++i)
		WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) (smb_base + SMBGLSTS)); WAIT; WAIT;
		if (dat == SMBHSTS_BUSY || dat == SMBHSTS_TIMEOUT)
			goto top;
		if (dat & (SMBHSTS_DONE | SMBHSTS_ALLERR))
			break;
	}
#ifdef SMB_DEBUG
if(i != LOOP_COUNT)
fprintf(stderr,
"   fx:Writeword: addr=0x%02X, cmd=0x%02X flag = 0x%02X, loop#:%04d\n",
addr, cmd, dat, i);
#endif
	if (dat == SMBHSTS_DONE)
		return 0;
	else
		return -1;
}

struct smbus_io smbus_amd8 = {
	readbyte, readword, writebyte, writeword
};
