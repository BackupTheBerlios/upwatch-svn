/*
 * Natinal Semiconductor LM80 hardware monitor chip
 *
 ***************************************************************
 * Before calling these routines, one must call method->Open() *
 * After calling these routines, one must call method->Close() *
 ***************************************************************
 *

National Semiconductor
         Chip         Temp    Volt    Fan     SMBus   IOport
        lm80           1       7       2       yes     no

 *
 *	Copyright Shin-ichi Nagamura
 */


#include	<stdio.h>
#include	"sensors.h"

/* external (global) data */
extern int smb_slave;
extern LM_METHODS method_smb;


#undef	DEBUG

#define	LM80_ADDR_START		0x50	/*0x50-0x5E*/
#define	LM80_ADDR_END		0x5E

#define	LM80_CONFIG			0x00
#define	LM80_ISR1			0x01
#define	LM80_ISR2			0x02
#define	LM80_FANDIV			0x05
#define	LM80_RESOLUTION		0x06
#define	LM80_IN0			0x20
#define	LM80_TEMPERATURE	0x27
#define	LM80_FANRPM1		0x28
#define	LM80_FANRPM2		0x29

#define	LM80_CONFIG_START	0x01
#define	LM80_CONFIG_INTEN	0x02
#define	LM80_CONFIG_INTOPEN	0x04
#define	LM80_CONFIG_INTCLR	0x08
#define	LM80_CONFIG_RESET	0x10
#define	LM80_CONFIG_CHACLR	0x20
#define	LM80_CONFIG_GPO		0x40
#define	LM80_CONFIG_INIT	0x80

#define	LM80_R06_OSACTHI	0x02
#define	LM80_R06_TEMPRES	0x08
#define	LM80_R06_TEMP11		0xF0
#define	LM80_R06_TEMP8		0x80

static	int		lm80_probe(LM_METHODS *);
static	float	lm80_temp(LM_METHODS *, int);
static	int		lm80_fanrpm(LM_METHODS *, int);
static	float	lm80_volt(LM_METHODS *, int);

#if defined(DEBUG)
static	void	dumpreg( void );
#endif	/*DEBUG*/

SENSOR lm80 = {
	"Nat.Semi.Con. Chip LM80",
	lm80_probe,
	lm80_temp,
	lm80_volt,
	lm80_fanrpm
};


#define LM80_chkRegNum 20

/* Register checked for probing */
static int chkReg[] = {
	0x00, 0x01, 0x02, 0x04,
	0x25, 0x26, 0x27, 0x29,
	0x2A, 0x2B, 0x2C, 0x2D,
	0x32, 0x33, 0x34, 0x35,
	0x36, 0x37, 0x38, 0x39,
	0x3A, 0x3B, 0x3C, 0x3D,
	-1 };


/*
 *  return 0 if not probed
 */
static	int     lm80_probe(LM_METHODS *method)
{
	int n, dat, reg, save;

	if( method != &method_smb )
		return 0;

	save = smb_slave;

	for (n = LM80_ADDR_START; n <= LM80_ADDR_END;) {
		if (!(smb_slave = get_smb_slave(n, LM80_ADDR_END)))
			goto ret0;
		else {
			if (method->Read(LM80_ISR2) & 0xC0) {
				n = smb_slave + 2;
				continue;
			}
			for (reg = 0x2A; reg <= 0x3D; reg++) {
				dat = method->Read(reg);
				if (method->Read(reg + 0x40) != dat ||
				    method->Read(reg + 0x80) != dat ||
				    method->Read(reg + 0xC0) != dat)
					break;
			}
			if (reg > 0x3D
				&& chkReg_Probe(smb_slave, "Probing LM80 chip:\n",
						chkReg, method) >= LM80_chkRegNum)
					goto ret1;
			else
				n = smb_slave + 2;
		}
	}

ret0:
	smb_slave = save;
	return 0;
ret1:
	if((method->Read(LM80_CONFIG ) & LM80_CONFIG_START) == 0) {
		method->Write(LM80_CONFIG, LM80_CONFIG_RESET);
		method->Write(LM80_CONFIG, LM80_CONFIG_START);
	}

	method->Write(LM80_FANDIV,
		(method->Read(LM80_FANDIV) & 0x3C) | 0x40);
	method->Write(LM80_RESOLUTION,
		LM80_R06_TEMPRES | LM80_R06_OSACTHI);

#if defined(DEBUG)
	dumpreg();
#endif	/*DEBUG*/

	kill_smb_slave(smb_slave);
	return 1;
}


/*!
 *	\retval	0xFFFF	no sensor
 *	\retval	other	temperature
 *  no = 0,1,2,...  
 */
static	float	lm80_temp(LM_METHODS *method, int no)
{
	int		reg, val, sft;
	float	ret;

	if (no != 0)
		return 0xFFFF;

	reg	= method->Read(LM80_RESOLUTION);
	val	= method->Read(LM80_TEMPERATURE);

	sft	= ((( reg & LM80_R06_TEMPRES ) >> 3 ) * 3 ) + 1;

	val	<<= sft;
	val	|= reg >> (8 - sft);

	ret	= (float)val;
	if (reg & LM80_R06_TEMPRES)
		ret	*= 0.0625;
	else
		ret	*= 0.5;

	return ret;
}



/*!
 *	\retval	0x0000FFFF	no sensor
 *  no = 0,1,2,...  
 */
static	float	lm80_volt(LM_METHODS *method, int no)
{
	const	float	r1[7]	= { 23.7, 23.7, 22.1, 24,  160,   27, 180 };
	const	float	r2[7]	= { 75,   75,   30,   14.7, 30.1, 3.4, 42.2	};
	float	vout, val;

	if( no < 0 || 6 < no )
		return 0xFFFF;

	vout  = method->Read(LM80_IN0 + no);
	vout *= 0.01;	/*LSB is 10mv*/

	/*
	 *VOUT = ( VCC * R2 ) / ( R1 + R2 )
	 *VCC  = ( VOUT * ( R1 + R2 )) / R2
	 */
	val	= ( vout * ( r1[no] + r2[no] )) / r2[no];
	if( no >= 5 )
		val	= ( val - 5 ) * -1;
/*
 *			((( vout * ( r1[no] + r2[no] )) / r2[no] ) - 5 ) * -1;
 */

	return val;
}


/*!
 *	\retval	0x0000FFFF	no sensor
 *	\retval	0x00010000	unknown (maybe no sensor)
 *  no = 0,1,2,...  
 */
static	int		lm80_fanrpm(LM_METHODS *method, int no)
{
	int		reg, val;
	int		div, sft;
	long	rpm;

#if defined(DEBUG)
printf("lm80_fanrpm(): no=%d\n", no);
#endif	/*DEBUG*/
	if (no < 0 || 1 < no)
		return 0xFFFF;

	reg	= method->Read(LM80_FANDIV);
	val	= method->Read(LM80_FANRPM1 + no);
#if defined(DEBUG)
printf("lm80_fanrpm(): reg=0x%02X, val=0x%02X\n", reg, val);
#endif	/*DEBUG*/
	sft	= (no + 1) * 2;
	div	= (reg >> sft) & 0x03;
#if defined(DEBUG)
printf( "lm80_fanrpm(): sft=%d, div=%d\n", sft, div );
#endif	/*DEBUG*/

	if (val == 0xFF) {
		if (div < 3) {
			reg	+= 1 << sft;
			(*method->Write)(LM80_FANDIV, reg);
		}
#if defined(DEBUG)
printf( "lm80_fanrpm(): write new reg 0x%02X\n", reg );
#endif	/*DEBUG*/
		return 0x10000;
	}

#if defined(DEBUG)
printf( "lm80_fanrpm(): val=%d, div=%d\n", val, div );
#endif	/*DEBUG*/
	rpm		= 1350000 / ( val * ( 1 << div ));
	return rpm;
}


#if defined(DEBUG)
static  void    dumpreg(void)
{
	int cmd;

	printf( "SlaveAddress=0x%04X", smb_slave );

#if 0
	printf( "Reg00 :" );
	for( cmd=0; cmd<=6; cmd++ )
		printf( " 0x%02X", (*method_smb.Read)(cmd));
#endif

	for( cmd=0x00; cmd<=0x7F; cmd++ ) {
		if(( cmd & 0x0F ) == 0 )
			printf( "\nReg%02X :", cmd );
		printf("%c%02X", ((cmd&0x0F)==8)?'-':' ',(*method_smb.Read)(cmd));
	}
	printf("\n");
}
#endif  /*DEBUG*/
