
/* SMB handling routines for AMD7xx, by YRS 2002.07.
	Information on how to access SMBus is provided
	by ":p araffin.(Yoneya)", MANY THANKS!!

	Common to AMD756/766/768 and NVidia nForce

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
#define SMBHADDR	0x4
#define SMBHDATA	0x6
#define SMBHCMD 	0x8

/* status flag */
#define SMBHSTS_TOSTS	0x20	/* ? not used */
#define SMBHSTS_DONE	0x10	/* host cycle (command) completed */
#define SMBHSTS_BUSY	0x08	/* host busy */
#define SMBHSTS_ERROR	0x04	/* device error */
#define SMBHSTS_COLLID	0x02	/* bus collision */
#define SMBHSTS_ABORT	0x01	/* abort */
#define SMBHSTS_CLEAR	(SMBHSTS_TOSTS|SMBHSTS_DONE|SMBHSTS_ERROR|\
							SMBHSTS_COLLID|SMBHSTS_ABORT)	/* clear status*/

/* control command number */
#define SMBHCTRL_ABORT	0x20	/* abort */
#define SMBHCTRL_START	0x08	/* start command */
#define SMBHCTRL_WORD	0x03	/* word I/O */
#define SMBHCTRL_BYTE	0x02	/* byte I/O */


static int readbyte(int smb_base, int addr, int cmd)
{
	u_char saddr = 2*(addr/2);
	u_short dat;
	int i;

top:
	OUTw((u_short) smb_base, SMBHSTS_CLEAR); WAIT; WAIT;
	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INw((u_short) smb_base); WAIT; WAIT;
		OUTw((u_short) smb_base, dat); WAIT; WAIT;
		if (!(dat & (SMBHSTS_COLLID | SMBHSTS_BUSY)))
			goto step;
		if (!(dat & SMBHSTS_BUSY))
			break;
	}
	OUTw((u_short) (smb_base + SMBHCTRL), SMBHCTRL_ABORT); WAIT; WAIT;

step:
	OUTw((u_short) (smb_base + SMBHADDR), (saddr | LSB)); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTw((u_short) (smb_base + SMBHCTRL),
		(SMBHCTRL_START | SMBHCTRL_BYTE)); WAIT; WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INw((u_short) smb_base); WAIT; WAIT;
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
		dat = INw((u_short) (smb_base + SMBHDATA)); WAIT; WAIT;
		return (dat & 0x00FF);
	} else
		return -1;
}

static int readword(int smb_base, int addr, int cmd)
{
	u_char saddr = 2*(addr/2);
	u_short dat;
	int i;

top:
	OUTw((u_short) smb_base, SMBHSTS_CLEAR); WAIT; WAIT;
	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INw((u_short) smb_base); WAIT; WAIT;
		OUTw((u_short) smb_base, dat); WAIT; WAIT;
		if (!(dat & (SMBHSTS_COLLID | SMBHSTS_BUSY)))
			goto step;
		if (!(dat & SMBHSTS_BUSY))
			break;
	}
	OUTw((u_short) (smb_base + SMBHCTRL), SMBHCTRL_ABORT); WAIT; WAIT;

step:
	OUTw((u_short) (smb_base + SMBHADDR), (saddr | LSB)); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTw((u_short) (smb_base + SMBHCTRL),
		(SMBHCTRL_START | SMBHCTRL_WORD)); WAIT; WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INw((u_short) smb_base); WAIT; WAIT;
		if (dat & SMBHSTS_COLLID)
			goto top;
		if (!(dat & SMBHSTS_BUSY) && (dat & (SMBHSTS_DONE | SMBHSTS_ERROR)))
			break;
	}
	if (dat & SMBHSTS_DONE) {
		dat = INw((u_short) (smb_base + SMBHDATA)); WAIT; WAIT;
		return (dat & 0xFFFF);
	} else
		return -1;
}

static int writebyte(int smb_base, int addr, int cmd, int value)
{
	u_char saddr = 2*(addr/2);
	u_short dat;
	int i;

	OUTw((u_short) smb_base, SMBHSTS_CLEAR); WAIT; WAIT;
	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INw((u_short) smb_base); WAIT; WAIT;
		OUTw((u_short) smb_base, dat); WAIT; WAIT;
		if (!(dat & (SMBHSTS_COLLID | SMBHSTS_BUSY)))
			goto step;
		if (!(dat & SMBHSTS_BUSY))
			break;
	}
	OUTw((u_short) (smb_base + SMBHCTRL), SMBHCTRL_ABORT); WAIT; WAIT;

step:
	OUTw((u_short) (smb_base + SMBHADDR), saddr); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTw((u_short) (smb_base + SMBHDATA), (u_char) value); WAIT; WAIT;
	OUTw((u_short) (smb_base + SMBHCTRL),
		(SMBHCTRL_START | SMBHCTRL_BYTE)); WAIT; WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INw((u_short) smb_base); WAIT; WAIT;
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
	u_char saddr = 2*(addr/2);
	u_short dat;
	int i;

	OUTw((u_short) smb_base, SMBHSTS_CLEAR); WAIT; WAIT;
	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INw((u_short) smb_base); WAIT; WAIT;
		OUTw((u_short) smb_base, dat); WAIT; WAIT;
		if (!(dat & (SMBHSTS_COLLID | SMBHSTS_BUSY)))
			goto step;
		if (!(dat & SMBHSTS_BUSY))
			break;
	}
	OUTw((u_short) (smb_base + SMBHCTRL), SMBHCTRL_ABORT); WAIT; WAIT;

step:
	OUTw((u_short) (smb_base + SMBHADDR), saddr); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTw((u_short) (smb_base + SMBHDATA), (u_short) value); WAIT; WAIT;
	OUTw((u_short) (smb_base + SMBHCTRL),
		(SMBHCTRL_START | SMBHCTRL_WORD)); WAIT; WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INw((u_short) smb_base); WAIT; WAIT;
		if (!(dat & SMBHSTS_BUSY) && (dat & (SMBHSTS_DONE | SMBHSTS_ERROR)))
			break;
	}
	if (dat & SMBHSTS_DONE)
		return 0;
	else
		return -1;
}

struct smbus_io smbus_amd = {
	readbyte, readword, writebyte, writeword
};
