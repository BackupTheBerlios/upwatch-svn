
/* SMB handling routines for ALi 1535D+, by YRS 2002.07.
	Information on how to access SMBus is provided
	by ":p araffin.(Yoneya)", MANY THANKS!!


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
#define SMBHSTS 	0x0 /* SMBus host/slave status register */
#define SMBHCTRL	0x1 /* SMBus host/slave control register */
#define SMBSTART	0x2 /* start to generate programmed cycle */
#define SMBHADDR	0x3 /* host address register */
#define SMBHDATA0	0x4 /* data A register for host controller */
#define SMBHDATA1	0x5 /* data B register for host controller */
#define SMBHCMD     0x7 /* command register for host controller */

/* status flag */
#define SMBHSTS_WRONG	0x00	/* something wrong, restart */
#define SMBHSTS_IDLE	0x04	/* host idle, can put commands */
#define SMBHSTS_BUSY	0x08	/* host busy */
#define SMBHSTS_DONE	0x10	/* command completed */
#define SMBHSTS_DEVERR	0x20	/* device error */
#define SMBHSTS_BUSERR	0x40	/* bus collision or no response */
#define SMBHSTS_FAILED	0x80	/* failed bus transaction */
#define SMBHSTS_ALLERR	0xE0	/* all the bad error bits */
#define SMBHSTS_CLEAR	0xFF	/* clear status */

/* control command number */
#define SMBHCTRL_HRESET	0x04	/* host reset */
#define SMBHCTRL_BRESET	0x08	/* entire bus reset */
#define SMBHCTRL_RESET	0x0C	/* reset */
#define SMBHCTRL_BYTE	0x20	/* write/read byte */
#define SMBHCTRL_WORD	0x30	/* write/read word */


static int readbyte(int smb_base, int addr, int cmd)
{
	u_char dat, saddr = 2*(addr/2);
	int i;

top:
	for (i = 0; i < LOOP_COUNT; ++i) {
		OUTb((u_short) smb_base, SMBHSTS_CLEAR); WAIT; WAIT;
		dat = INb((u_short) smb_base); WAIT; WAIT;
		if (dat & SMBHSTS_IDLE)
			goto step;
	}
	OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_RESET); WAIT; WAIT;

step:
	OUTb((u_short) (smb_base + SMBHADDR), (saddr | LSB)); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_BYTE); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBSTART), 0xFF); WAIT; WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) smb_base); WAIT; WAIT;
		if (!(dat & SMBHSTS_BUSY))
			break;
	}
#ifdef SMB_DEBUG
if (debug_flag > 1)
fprintf(stderr, "   Readbyte: flag = 0x%02X, loop#:%04d", dat, i);
#endif
	if (dat == SMBHSTS_WRONG) {
		OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_RESET); WAIT; WAIT;
#ifdef SMB_DEBUG
if (debug_flag > 1)
fprintf(stderr, "\n   OH!  wrong   ! = 0x%02X\n", dat);
#endif
		goto top;	
	} else if (dat & SMBHSTS_DONE) {
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
	for (i = 0; i < LOOP_COUNT; ++i) {
		OUTb((u_short) smb_base, SMBHSTS_CLEAR); WAIT; WAIT;
		dat = INb((u_short) smb_base); WAIT; WAIT;
		if (dat & SMBHSTS_IDLE)
			goto step;
	}
	OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_RESET); WAIT; WAIT;

step:
	OUTb((u_short) (smb_base + SMBHADDR), (saddr | LSB)); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_WORD); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBSTART), 0xFF); WAIT; WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) smb_base); WAIT; WAIT;
		if (!(dat & SMBHSTS_BUSY))
			break;
	}
	if (dat == SMBHSTS_WRONG) {
		OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_RESET); WAIT; WAIT;
		goto top;	
	} else if (dat & SMBHSTS_DONE) {
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

	for (i = 0; i < LOOP_COUNT; ++i) {
		OUTb((u_short) smb_base, SMBHSTS_CLEAR); WAIT; WAIT;
		dat = INb((u_short) smb_base); WAIT; WAIT;
		if (dat & SMBHSTS_IDLE)
			goto step;
	}
	OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_RESET); WAIT; WAIT;

step:
	OUTb((u_short) (smb_base + SMBHADDR), saddr); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHDATA0), (u_char) value); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_BYTE); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBSTART), 0xFF); WAIT; WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) smb_base); WAIT; WAIT;
		if (!(dat & SMBHSTS_BUSY))
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

	for (i = 0; i < LOOP_COUNT; ++i) {
		OUTb((u_short) smb_base, SMBHSTS_CLEAR); WAIT; WAIT;
		dat = INb((u_short) smb_base); WAIT; WAIT;
		if (dat & SMBHSTS_IDLE)
			goto step;
	}
	OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_RESET); WAIT; WAIT;

step:
	OUTb((u_short) (smb_base + SMBHADDR), saddr); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCMD), (u_char) cmd); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHDATA0), (u_char) (value & 0xFF)); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHDATA1), (u_char) (value >> 8)); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBHCTRL), SMBHCTRL_WORD); WAIT; WAIT;
	OUTb((u_short) (smb_base + SMBSTART), 0xFF); WAIT; WAIT;

	for (i = 0; i < LOOP_COUNT; ++i) {
		dat = INb((u_short) smb_base); WAIT; WAIT;
		if (!(dat & SMBHSTS_BUSY))
			break;
	}
	if (dat & SMBHSTS_DONE)
		return 0;
	else
		return -1;
}

struct smbus_io smbus_ali = {
	readbyte, readword, writebyte, writeword
};
