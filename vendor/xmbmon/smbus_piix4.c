
/* SMB handling routines for PIIX4, by YRS 2001.08.
	Information on how to access SMBus is provided
	by ":p araffin.(Yoneya)", MANY THANKS!!

	Common to PIIX4, ICHx, VIA686/VT8233

	SMBus IO method:

	smb_base  : Base Address
	addr      : Slave Device Address
	cmd       : Command Register 

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

#define LSB 	0x01

#define LOOP_COUNT	0x2000

/* command address */
#define SMBHSTS 	0x0
#define SMBHCTRL	0x2
#define SMBHCMD 	0x3
#define SMBHADDR	0x4
#define SMBHDATA0	0x5
#define SMBHDATA1	0x6

/* status flag */
#define SMBHSTS_FAILED	0x10	/* failed bus transaction */
#define SMBHSTS_COLLID	0x08	/* bus collision */
#define SMBHSTS_ERROR	0x04	/* device error */
#define SMBHSTS_DONE	0x02	/* command completed */
#define SMBHSTS_BUSY	0x01	/* host busy */
#define SMBHSTS_CLEAR	(SMBHSTS_FAILED|SMBHSTS_COLLID|\
							SMBHSTS_ERROR|SMBHSTS_DONE)	/* clear status */

/* control command number */
#define SMBHCTRL_START	0x40	/* start command */
#define SMBHCTRL_BYTE	0x08	/* byte I/O */
#define SMBHCTRL_WORD	0x0C	/* word I/O */
#define SMBHCTRL_KILL	0x02	/* stop the current transaction */
#define SMBHCTRL_ENABLE	0x01	/* enable interrupts */


static int readbyte(int smb_base, int addr, int cmd)
{
	u_char dat, saddr = 2*(addr/2);
	int i;

top:
	OUTb((u_short) smb_base, SMBHSTS_CLEAR); WAIT; WAIT;
	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) smb_base); WAIT; WAIT;
		OUTb((u_short) smb_base, dat); WAIT; WAIT;
		if (!(dat & (SMBHSTS_COLLID | SMBHSTS_BUSY)))
			goto step;
		if (!(dat & SMBHSTS_BUSY))
			break;
	}
	OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_KILL); WAIT; WAIT;

step:
	OUTb((u_short) (smb_base + SMBHADDR), (saddr | LSB)); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCTRL),
		(SMBHCTRL_START | SMBHCTRL_BYTE)); WAIT; WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) smb_base); WAIT; WAIT;
		if (dat & SMBHSTS_COLLID) {
#ifdef SMB_DEBUG
if (debug_flag > 1)
fprintf(stderr, "\n   OH! collision! = 0x%02X\n", dat);
#endif
			goto top;
		}
		if (!(dat & SMBHSTS_BUSY) && (dat & (SMBHSTS_DONE | SMBHSTS_ERROR)))
			break;
	}
#ifdef SMB_DEBUG
if (debug_flag > 1)
fprintf(stderr, "   Readbyte: flag = 0x%02X, loop#:%04d", dat, i);
#endif
	if (dat & SMBHSTS_DONE) {
		dat = INb((u_short) (smb_base + SMBHDATA0)); WAIT; WAIT;
		return (dat & 0xFF);
	} else
		return -1;
}

static int readword(int smb_base, int addr, int cmd)
{
	u_char dat, saddr = 2*(addr/2);
	int i;

top:
	OUTb((u_short) smb_base, SMBHSTS_CLEAR); WAIT; WAIT;
	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) smb_base); WAIT; WAIT;
		OUTb((u_short) smb_base, dat); WAIT; WAIT;
		if (!(dat & (SMBHSTS_COLLID | SMBHSTS_BUSY)))
			goto step;
		if (!(dat & SMBHSTS_BUSY))
			break;
	}
	OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_KILL); WAIT; WAIT;

step:
	OUTb((u_short) (smb_base + SMBHADDR), (saddr | LSB)); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCTRL),
		(SMBHCTRL_START | SMBHCTRL_WORD)); WAIT; WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) smb_base); WAIT; WAIT;
		if (dat & SMBHSTS_COLLID)
			goto top;
		if (!(dat & SMBHSTS_BUSY) && (dat & (SMBHSTS_DONE | SMBHSTS_ERROR)))
			break;
	}
	if (dat & SMBHSTS_DONE) {
		i = INb((u_short) (smb_base + SMBHDATA1)); WAIT; WAIT;
		dat = INb((u_short) (smb_base + SMBHDATA0)); WAIT; WAIT;
		return (((i << 8) + dat) & 0xFFFF);
	} else
		return -1;
}

static int writebyte(int smb_base, int addr, int cmd, int value)
{
	u_char dat, saddr = 2*(addr/2);
	int i;

	OUTb((u_short) smb_base, SMBHSTS_CLEAR); WAIT; WAIT;
	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) smb_base); WAIT; WAIT;
		OUTb((u_short) smb_base, dat); WAIT; WAIT;
		if (!(dat & (SMBHSTS_COLLID | SMBHSTS_BUSY)))
			goto step;
		if (!(dat & SMBHSTS_BUSY))
			break;
	}
	OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_KILL); WAIT; WAIT;

step:
	OUTb((u_short) (smb_base + SMBHADDR), saddr); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHDATA0), (u_char) value); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCTRL),
		(SMBHCTRL_START | SMBHCTRL_BYTE)); WAIT; WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) smb_base); WAIT; WAIT;
		if (!(dat & SMBHSTS_BUSY) && (dat & (SMBHSTS_DONE | SMBHSTS_ERROR)))
			break;
	}
	if (dat & SMBHSTS_DONE)
		return 0;
	else
		return -1;
}

static int writeword(int smb_base, int addr, int cmd, int value)
{
	u_char dat, saddr = 2*(addr/2);
	int i;

	OUTb((u_short) smb_base, SMBHSTS_CLEAR); WAIT; WAIT;
	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) smb_base); WAIT; WAIT;
		OUTb((u_short) smb_base, dat); WAIT; WAIT;
		if (!(dat & (SMBHSTS_COLLID | SMBHSTS_BUSY)))
			goto step;
		if (!(dat & SMBHSTS_BUSY))
			break;
	}
	OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_KILL); WAIT; WAIT;

step:
	OUTb((u_short) (smb_base + SMBHADDR), saddr); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHDATA0), (u_char) (value & 0xFF)); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHDATA1), (u_char) (value >> 8)); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCTRL),
		(SMBHCTRL_START | SMBHCTRL_WORD)); WAIT; WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) smb_base); WAIT; WAIT;
		if (!(dat & SMBHSTS_BUSY) && (dat & (SMBHSTS_DONE | SMBHSTS_ERROR)))
			break;
	}
	if (dat & SMBHSTS_DONE)
		return 0;
	else
		return -1;
}

struct smbus_io smbus_piix4 = {
	readbyte, readword, writebyte, writeword
};
