 *********************************************************
    Mother Board Monitor Program for X Window System

        XMBmon ver.2.03

    for FreeBSD (and for Linux and NetBSD/OpenBSD).
 *********************************************************


<< 0. What's this software >>

  Recent motherboards have functionalities to monitor the CPU
temperatures and the frequency of CPU cooling fans etc.  Although
some programs utilizing these hardware monitoring facilities have been
developed for the Microsoft Windows platforms, no programs seem to
exist for PC-UNIX and the X Windows System platforms.  Thus, I have
tried to make small programs.  They have only least functionalities,
the one "mbmon" used at the command line reports the temperatures,
voltages and rpm (rounds per minute) of cooling fans, and the
other "xmbmon" displays the three temperatures and a core voltage
as simple curves.


<< 1. Changes in the new version >>
------------------------------------------------------------------
(ver. 2.02 ---> ver. 2.03)

 * Genesys Logic GL512SM and GL520SM sensors are supported.

 * Intel ICH5 SMBus access supported.

 * LM85, Analog Devices ADM1024/1025/1027/ADT7463,
  and SMSC EMC6D10X sensors are supported.

 * Analog Devices ADM1020/1021/1023 temperature sensors are supported.

 * A log standing bug from version 2.00 in the treatment of
  fan-divisor for the IT87xxF sensor chip is fixed.

 * Others: Fixing small bugs,  clean-up the code.
------------------------------------------------------------------
(ver. 2.01 ---> ver. 2.02)

 * NetBSD/OpenBSD support is added (by a contribution of a patch
  given by Stephan Eisvogel). "./configure; make" just works.

 * The AMD8111 and NVidia nForce2 SMBus access is supported
  (by information from Alex van Kaam).

 * National Semiconductor LM90 temperature sensor is supported.

 * Winbond W83L784R, W83L785R, W83L785TS-S sensors are supported.

 * The case of two sensor chips are supported
   (see section 4 How to use for details).
------------------------------------------------------------------
(ver. 2.00 ---> ver. 2.01)

 * "mbmon" can be executed as a daemon program (contributed patch
  by Jean-Marc Zucconi).  This make it possible to get output of
  "mbmon" over network by executing telnet command
  (see section 4 How to use).

 * Improved treatment of ASUS ASB100 ("Bach") sensor chip (by
  information from Alex van Kaam).  The temperature of an internal
  Athlon XP diode is obtained (as a 2nd temp.) in ASUS motherboard
  of A7V333/A7V8X.

 * ASUS Mozart-2 sensor chip is supported (by information from
  Alex van Kaam).

 * Fixed the issue that "xmbmon" cannot read NFS mounted ~/.Xauthority
  file (contributed patch by Kazushi NAKAMURA).
------------------------------------------------------------------
(ver. 2.00)

 * Few more hardware monitor chips are supported; National
  Semiconductor LM75, LM80 (by Shin-ichi Nagamura), and ASUS
  ASB100 Bach.

 * Few more SMBus interface chipsets are supported; AMD7xx,
  NVidia nForce, and ALi MAGiK1/AladdinPro.

 * More powerful automatic detection of supported hardware monitor
  chips is provided, and one can specify a chip name for probing it.

 * Special Treatment for Tyan's Tiger MP/MPX motherboard is supported.

 * Further variety of output of the "mbmon" program is supported for
  purpose of utilizing its output for more fancy graphic programs rather
  than the "xmbmon", like MRTG or rrdtool.  This time output for rrdtool
  and convenient perl script is provided (by Shin-ichi Nagamura).

 * One can specify the device file number X (/dev/smbX) for users who
  has more than two devices on SMBus with FreeBSD SMBus device driver
  (This is only for FreeBSD, not for Linux).

 * The whole program has been almost completely rewritten to make it
  easier for supporting new hardware monitor chips and/or new SMBus
  interfaces (see 00READEtech.txt).

 * The output values of negative voltages are changed partly, so they 
  may not coincide with those of the previous version.

 * Others: Fixing small bugs.


<< 2. Supported hardware monitor chips >>

  The following hardware monitor chips are supported and the programs
will work if your motherboard uses one of them or their compatible chip:

   National Semiconductor co.        LM78/LM79, LM75, LM90, LM80, LM85
   WinBond co.                       W83781D, W83782D, W83783S,
                                     W83627HF, W83697HF built-in,
                                     W83L784R, W83L785R, W83L785TS-S
   ASUSTek co.                       AS99127F, ASB100 (Bach),
                                     ASM58 etc. (Mozart-2)
   VIA Technology co.                VT82C686A/B built-in
   Integrated Technology Express co. IT8705F, IT8712F built-in
   Genesys Logic                     GL518SM, GL520SM
   Analog Devices                    ADM1027, ADT7463, ADM1020/1021/1023
   Standard Microsystem co.          EMC6D100/101


<< 3. Supported SMBus interface chipsets >>

  There are several methods of the interface to hardware monitor
chips: one using ISA-IO port, or using specific method for VIA
82C686A/B chip (see below for details).  But the most common way
is the one through the so-called SMBus (System Management Bus),
and the IO port is different for different SMBus interface chips.
In this version, the following chips (the south bridge of the
chipset) are supported:

   Intel: PIIX4 (440BX), ICH, ICH0, ICH2, ICH3, ICH4,
                         ICH5 (810, 815, ...)
   VIA: VT82C596/B, VT82C686A/B (KT133/A), VT8233A/C (KT266/A/KT333),
                    VT8235(KT400)
   AMD: AMD756(AMD750), AMD766(AMD760), AMD768(AMD760MP), AMD8111
   NVidia: nForce, nForce2
   Acer Lab. Inc.(ALi): M1535D+(MAGiK1/AladdinPro)

Here, I must confess that I myself did not check all the motherboards
having the chipsets above.  So I cannot guarantee the programs work for
all motherboards using the following chipsets.

  The motherboards, which are checked by myself or beta testers and
are reported to work by users:

   ABIT: VP6(ApolloPro133A(VIA686B)), KT7A(KT133A),
         KG7(AMD761+VIA686A), NF7(nForce2/ISA+W83627HF)
   ASUS: P2B,P2B-F,P2B-B(440BX+W83781D), P3B-F(440BX+AS99127F),
         K7VM(KT133+W83782D), A7V,A7V133(KT133/A+AS99127F),
         A7A266(ALi M1535D++AS99127F), A7V266(KT266/A+AS99127F),
         A7N266(nForce+AS99127F), A7M266-D(AMD768+AS99127F),
         A7V333(KT333+ASB100), A7V8X(KT400+ASB100),
         A7N8X(nForce2+ASB100+W83L785TS-S),
         CUSL(Intel815E+AS99127F), TUSI-M(SiS630/ISA+IT8705F),
         P4B533-VM(Intel845(ICH4)+ASM58), P4PE(Intel845PE(ICH4)+ASM100)
   EpoX: BX6SE(440BX/ISA+W83782D), 8KHA+(KT266A/ISA+W83697HF)
   ECS:  K7S5A(Sis735/ISA+IT8705F), D6VAA(AppoloPro133A(VIA686B))
   Soltek: SL75DRV2(KT266+IT8705F)
   Freeway: FW-6280BXDR/155(440BX+W83781D/W83782S)
   Aopen: MX3S (i815E(ICH2)+W83627H), MX36LE (PLE133(VIA686B))
   Shuttle: FV24 (PLE133(VIA686B))
   MSI:  K7N2 Delta-L(nForce2/ISA+W83627HF)
   Tyan: TigerMP/MPX(S2460/S2462)(AMD766+W83782D+W83627HF)[Note],
         Trinity 100AT(VIA586A(viapm)+GL518SM)
   Gigabyte: GA-7VAXP (KT400/ISA+IT8705F+LM90),
             GA-SINXP-1394(SiS655/ISA+IT8705F)
   Leadtek: K7NCR18D (nForce2+W83783S+W83L785TS-S)
   Albatron: KX400+Pro (KT333/ISA+W83697HF+W83L785TS-S)
   Aopen: AX4GER(Intel845GE(ICH4)/ISA+W83627HF)
   Intel: S845WD1(Tentel845E(ICH4)+LM85)

[Note] As for Tyan TigerMP/MPX, see [Note] at the end of 6.Option.


<< 4. How to use >>

  By using "./configure" and "make" under the directory where this
package is unpacked, two binary programs

      mbmon      Mother Board Monitor for tty-terminal
      xmbmon     Mother Board Monitor for X

will be generated.

((NOTICE)) These programs access SMBus or ISA-IO port without any kind
   of checking.  It is, therefore, very dangerous and may cause
   system-crash in worst cases.  Especially, accesses of IO port 0x295,
   0x296 may conflict NE2000 boards' IO ports.  Users are requested to
   be sure that access to those ports are allowed.  I do not take any
   responsibilities for problems caused by using these programs.

  If you succeed compiling the programs, then it is better first to
check they will work on your motherboard.  Switch to the root user,
and execute "mbmon" like this,

   # ./mbmon -d   (or ./mbmon -d -A   see below)

in the debug mode.  If you do NOT get the following error message,

   No Hardware Monitor found!!
   InitMBInfo: Undefined error: 0

but it displays information of the detected hardware monitor chip like
this,

   Using SMBus access method[VIA8233/A(KT266/KT333)]!!
   * Asus Chip, ASB100 found.

then the programs will most probably work.  Try to execute "mbmon"
now without "-d" option, and if the following output like this

   Temp.= 31.0, 37.0, 37.0; Rot.= 3970, 2576, 2700
   Vcore = 1.74, 1.74; Volt. = 3.38, 5.08, 12.40, -11.35, -4.90

comes out every five seconds, congratulation!; the programs work on
your system.  If only one temperature is displayed and you are sure
that your motherboard has a hardware monitoring facility of voltage
and fan speed, then try to add "-A" option (see the next paragraph
for explanation).  If it is working fine, just stop it by typing "^C"
(CTRL-C), and install these two programs (Only ^C can stop "mbmon").
Note that both programs, mbmon/xmbmon, access SMBus or ISA-IO port
so that they should sit in a directory in the PATH variable with
a "setuid root" permission.  An example of installation is written
in Makefile.  If you put "mbmon" in /usr/local/bin and "xmbmon" in
/usr/X11R6/bin, then just type

   # make install

and then they are installed properly.

  In some motherboard, there exist two sensor chips, one is for a primary
hardware monitor, and the other is a secondary one for measuring CPU
temperature.  From version 2.02, such a case is also supported.  In order
to see how many possible sensor chips are used in your motherboard,
execute "mbmon" with "-A" option in the following way:

   # ./mbmon -d -A

If the result is like this:

   Summary of Detection:
    * SMB monitor(s)[VT8233/A/8235(KT266/KT333/KT400)]:
     ** Winbond Chip W83L785TS-S found at slave address: 0x5C.
    * ISA monitor(s):
     ** Winbond Chip W83697HF found.

It means that the primary monitor chip is W83697HF with a secondary
sensor W83L785TS-S for CPU temperature (attached to the CPU on-die
diode sensor).  In this case, the default action is

   # ./mbmon

   Temp.= 76.0,  0.0,  0.0; Rot.=    0,    0,    0
   Vcore = 0.00, 0.00; Volt. = 0.00, 0.00,  0.00,   0.00,  0.00

   # ./mbmon -I

   Temp.= 41.0, 64.5,  0.0; Rot.= 4440,    0,    0
   Vcore = 1.66, 0.00; Volt. = 3.26, 4.76, 12.65, -12.20, -5.15

But, with "-A", these output is unified such as,

   # ./mbmon -A

   Temp.= 41.0, 64.5, 76.0; Rot.= 4440,    0,    0
   Vcore = 1.66, 0.00; Volt. = 3.26, 4.76, 12.65, -12.20, -5.15

where the appeared third temperature is the CPU temperature (76.0)
detected by the W83L785TS-S chip.  In this example, W83697HF has only
two temperature sensors in it, so that the third temperature is naturally
replaced with the value obtained by the extra sensor W83L785TS-S.  But
if the primary monitor chip can measure three temperatures, you have to
choose which one should be replaced with the secondary sensor by option
"-e [0|1|2]", which means the first, second, third temperature being
replaced, respectively (default is the third one).  In the present
implement, the sensor chips treated as an extra sensor are the following:

     W83L785TS-S
	 LM90/ADM1020/ADM1021/ADM1023
	 LM75


  As I said, the programs access IO port directly, so even if the
program has a "setuid root" permission, they won't work on the system
where this IO port access is prohibited.  If the following message,

   InitMBInfo: Operation not permitted
   This program needs "setuid root"!!

comes on FreeBSD, the system security level is too strict and IO port
access is not allowed (see kern_securelevel_enable, kern_securelevel
in /etc/rc.conf).  Make your security level lower, or just give up
to use the programs.

  From version 2.01, "mbmon" can be executed in daemon mode, and one
can check output from an another machine over network.  By executing,

   # make -P port_no

at a remote host, which one wants to monitor, using telnet command
from other machines like,

   $ telnet hostname port_no
   Trying xxx.xxx.xxx.xxx...
   Connected to hostname.
   Escape character is '^]'.

   Temp.= 29.0, 25.0, 38.5; Rot.= 3750, 3688, 2410
   Vcore = 1.70, 1.70; Volt. = 3.31, 4.97, 12.40, -11.35, -4.87
   Connection closed by foreign host.

one can quite easily get output of "mbmon".

  It has been known that the output values of negative voltages (the
4th [-12V] and 5th [-5V] column of "Volt. = ...") are not precise,
depending on motherboards.  This is because the motherboard maker
does not follow the instruction of data sheet for each hardware
monitor chip.  So it is impossible to correct the inaccurate values.

  A motherboard of dual CPU (SMP) machines may have two hardware
monitor chips, one of which is connected to the SMBus and the other to
the ISA IO port (see 5. A bit of more details).  If the result seems
not proper, try to execute "mbmon" with -I option.

  For the Linux platforms -DLINUX option should be added to the DEFS
variable in Makefile.  If "configure" script failed to add it, do it
by yourself.  I do not have Linux machines available around me.
Therefore, I am afraid that the programs will not work, although
I have checked to compile and run on the Linux emulation environment
on FreeBSD.


<< 5. A bit of more details >>

  The first version of this software is programmed mainly to use
the Winbond W86781D chip as a hardware monitor chip of motherboard.
This chip can handle three temperatures (Temp0, Temp1, Temp2), seven
voltages (V0 - V6), and three FAN rotational speeds (Fan0, Fan1, Fan2).
The program, "mbmon", shows all the information in text on tty's, and
"xmbmon" shows Temp0,1,2 and V0(Vcore) graphically on X.  The units are
degree in Celsius (C) for temperatures [it is now possible to show them
in Fahrenheit (F) from version 1.07], Volt (V) for voltages, and
rotations per minute (rpm) for Fan rotation speeds.  Other chips may not
support all of them.  For example, VIA's VI82C686 will report on
three temperatures, five voltages, and two fan speeds.  The items
which are not supported by chip are displayed by "0.0" or "0"
in the "mbmon" output.  The program "mbmon" can take a input of
interval (in seconds, default value is 5 seconds) for showing the
output monitor information.

  In the case of "xmbmon", Temp0,1,2 are depicted with legends
"MB", "CPU", "chip", respectively.  They are X resources and can be
freely changed (see the included file, xmbmon.resources).  Note that
which temperature is assigned is different in each motherboard.
If these legends are not appropriate for your motherboard, change
them appropriately by yourself.  If your motherboard has only two
sensor and/or only two are connected, it is possible not to display
one of the three temperatures by setting "NOTEMP" (or blank '')
(see section 6. for details of options/resources ).

  If you do not know what kind of hardware monitor chips your mother
board has, then by using the following "debug" option

   # mbmon -d            or      # mbmon -D
   # xmbmon -debug               # xmbmon -DEBUG

information of the monitor chip would be detected(-D/-DEBUG is for
getting more detailed information).  You can judge, more or less,
whether these programs can run on your machine.  Among the supported
hardware monitor chips, LM78/79 has only Temp0 (measuring the temperature
of motherboard) occasionally with LM75 which can monitor only temperature
being used for CPU.  The W83783S chip does not support Temp2.  Some
motherboard may not use all Temp0/1/2 functionalities even though the
monitor chip has all.  Usually, Temp0 is for measuring motherboard's
temperature, which is about the room temperature and does not change.
Temp1,2 are only effective when they are connected to external sensor
chips explicitly in most cases.  But in recent motherboards (like ASUS
P3B-F) Temp1 is often connected to the inner sensor of Pentium II/III
CPU by default.

  Generally, recent motherboards provide two access methods to the
hardware monitor chip, namely those using the SMBus (System Management
Bus) and the old fashioned ISA-IO port.  New chips (like AS99127F
on ASUS P3B-F motherboard) would only allow the SMBus access method.
From the version 1.06 the program equipped its own access functions
to the SMBus, and it is possible to read data from sensor chips
directly through SMBus without any help of the OS supported device
drivers (see 3. Supported SMBus interface chipsets). The present version
(4.6R) of FreeBSD provides the following SMBus drivers, "intpm"(for
PIIX4 south chip of 440BX), "amdpm"(for AMD756[Note1]), "alpm"(for
ALi M15x3[Note2]), and "viapm"(for VIA586/596/686/8233).  One can
use these drivers in place of the SMBus access routines of the "xmbmon"
package by compiling it with -DSUMBUS_IOCTL macro of the variable DEFS
in the Makefile.  If the "./configure" process finds a device file
/dev/smb0, the program utilizes the ioctl calls to access SMBus through
the device driver.  The SMBus is one of general buses, and the hardware
monitor chip is not necessarily the only device using the bus.  For
example, the driver "bktr" for the Brooktree chip adopted many TV tuner
cards also uses /dev/smbX.  Thus, if there are some devices on the bus,
one must identify which device file, /dev/smbX, the hardware monitor
chip is using by checking the output of the "dmesg" message.  The system
will crash or reboot if a wrong device file number is used for "xmbmon"
(I have experienced this since I have a TV tuner card with the "bdtr0"
driver).  Therefor, a new option for specifying the device file number
X of /dev/smbX from the version 2.00, the -s/-smbdev option:

   # mbmon -s X[0-9]
   # xmbmon -smbdev X[0-9]

the default of which is X=0 (/dev/smb0).

  The built-in hardware monitor in the VIA's south chipset VT82C686A/B
has an another access method to the chip, by using a different ISA-IO
port access from that of Winbond chips.  Thus, including this for
VT82C686A/B, there are three methods to access the supported hardware
monitor chips.  I call this specific method for the VIA's chip, method V,
more general method using SMBus, method S, and the old fashioned ISA-IO
port method for the LM78/79 or Winbond chips, method I.  One can impose
the programs to use one of these three methods by giving a method option;

   # mbmon -[V|S|I|A]
   # xmbmon -method [V|S|I|A]

for method, V, S, I, respectively.  To check your motherboard supports
which access method, you just try to execute programs with this and the
debug option (-d).  If this method option is not supplied, the programs
try to check method V, S, I in this order, and automatically choose the
access method.  There are many more options for "mbmon/xmbmon", which
will be denoted by executing it with a help option (-h or -help). For
X, resources can also be used by writing them in user's .Xdefaults file.
They are explained in details in 6. Options, and X resources of "xmbmon".

  From version 2.00 another new option is supplied for specifying the
hardware monitor chip used in order to avoid mis-detection of hardware
monitor chips.  It is useful if the users know which monitor chips are
used on their motherboards.  Try to use this option if the automatic
detection fails.  The chip names one can specify is the following;

   # mbmon -p [winbond|wl784|via686|it87|lm80|lm90|lm75]
   # xmbmon -probe [winbond|wl784|via686|it87|lm80|lm90|lm75]

i.e. seven kinds of chips, which corresponds precisely to the following
monitor chips,

  winbond:   LM78/LM79,W83781D, W83782D, W83783S,W83627HF,W83697HF,
             AS99127F, ASB100
  wl784:     W83L784R, W83L785R, W83L785TS-S
  via686:    VT82C686A/B
  it87:      IT8705F, IT8712F
  lm80:      LM80
  lm90:      LM90
  lm75:      LM75


[Note1] "amdpm" driver recently has supported the SMBus access of
   NVidia nForce chipset (but it is not yet committed).  Moreover,
   a small patch make it possible to support AMD766/768 (see
   [FreeBSD-users-jp 68570]).  If you want more detailed information,
   please contact me directly.

[Note2] "alpm" driver supports a older ALi M15x3 chipset, but it is
   possible to support newer M1535D+ used in MAGiK1 or AladdinPro
   chipsets by a small patch.  If you want more detailed information,
   please contact me directly.


<< 6. Options, and X resources of "xmbmon" >>

  "mbmon" has the following options:

   # mbmon -h
MotherBoard Monitor, ver. 2.02 by YRS.
Usage: mbmon [options...] <seconds for sleep> (default 5 sec)
 options:
  -V|S|I: access method (using "VIA686 HWM directly"|"SMBus"|"ISA I/O port")
  -A: for probing all methods, all chips, and setting an extra sensor.
  -d/D: debug mode (any other options except (V|S|I) will be ignored)
 [-s [0-9]: for using /dev/smb[0-9]\n" ]<== compiled with -DSMBUS_IOCTL
  -e [0-2]: set extra temperature sensor to temp.[0|1|2] (need -A).
  -p chip: chip=winbond|wl784|via686|it87|lm80|lm90|lm75 for probing chips
  -Y: for Tyan Tiger MP/MPX motherboard
  -h: print help message(this) and exit
  -f: temperature in Fahrenheit
  -c count: repeat <count> times and exit
  -P port: run in daemon mode, using given port for clients
  -T|F [1-7]: print Temperature|Fanspeed according to following styles
        style1: data1\n
        style2: data2\n
        style3: data3\n
        style4: data1\ndata2\n
        style5: data1\ndata3\n
        style6: data2\ndata3\n
        style7: data1\ndata2\ndata3\n
  -r: print TAG and Value format
  -u: print system uptime
  -t: print present time
  -n|N: print hostname(long|short style)
  -i: print integers in the summary(with -T option)

  Here, -V|S|I is specifying the access method to the hardware monitor
chips mentioned above; -f is for showing temperatures in Fahrenheit
rather than Celsius; -d/D for debugging; -s is for specifying the
device file number X of /dev/smbX; -p is for specifying the monitor
chip probed; -h for showing help messages above.  The options below
-c are for displaying various multi-purpose output of the hardware
monitor: They are rather apparent from the message above; -c "count"
is for exit of execution after showing the output "count" times;
-P is, with giving port number, for executing "mbmon" in daemon mode
(see section 4. How to use); -T|F for output of temperature or fan
speed in the following "style"; -u, -t, -n|N for showing some machine
information, which is not related to the hardware monitor actually;
-i is for output in integer rather than floating point format.

  In version 2.02, new options -A and -e [0-2] are added in order to
show the temperature measured by an extra sensor chip.  -A is for
forced detection of extra sensors if possible.  -e [0|1|2] is for
replacing the first, second, third temperature with the value of
an extra sensor; default action is to replace the third one (-e 2).
See section 4 How to use for details.

  Now by using this strong output functionality above, one can utilize
outer programs of the "xmbmon" packages, something like MRTG or rrdtool.
Especially, an convenient perl script for rrdtool is contributed by
Shin-ichi Nagamura.  If it is installed in a proper path place,

   # mbmon-rrd.pl [rrdfile]

makes the data file for rrdtool quite easily.  Of course, one has to
install rrdtool package/port before using it.

  "xmbmon" is programmed with the X toolkit, and the standard X
resources (like geometry, font) can be used.  Other "xmbmon" specific
resources are the following (file "xmbmon.resources"):

XMBmon*count:   counts to check temp. etc in "sec" below (default:4)
XMBmon*sec:     period in seconds for plot one point (default:20)
XMBmon*wsec:    total period in seconds for window width (default:1800)
XMBmon*tmin:    lowest scale of temperature in degree C (default:10.0)
XMBmon*tmax:    highest scale of temperature in degree C (default:50.0)
XMBmon*vmin:    lowest scale of V-core voltage in V (default:1.80)
XMBmon*vmax:    highest scale of V-core voltage in V (default:2.20)
XMBmon*tick:    number of ticks for temp. and V-core (default:3)
XMBmon*cltmb:   color for plotting Temp0 (default:blue)
XMBmon*cltcpu:  color for plotting Temp1 (default:red)
XMBmon*cltcs:   color for plotting Temp2 (default:cyan)
XMBmon*clvc:    color for plotting Vcore (default:green)
XMBmon*cmtmb:   legend of Temp0, NOTEMP for not showing (default:\ MB)
XMBmon*cmtcpu:  legend of Temp1, NOTEMP for not showing (default:CPU)
XMBmon*cmtcs:   legend of Temp2, NOTEMP for not showing (default:chip)
XMBmon*cmvc:    legend of Vcore (default:Vc\ )
XMBmon*method:  access method (default:\ )
XMBmon*extratemp: temperature replaced with an extra sensor (default: 2)
XMBmon*smbdev:  number X of device file /dev/smbX (default:0)
XMBmon*fahrn:   temperature in Fahrenheit, True or False (default: False)
XMBmon*TyanTigerMP:  for Tyan Tiger MP/MPX motherboard (default:False)
XMBmon*probe:   chip name to be probed (default:\ )
XMBmon*label:   label (default: )
XMBmon*labelcolor: color of label (default: black)

  The default behavior is that every 20(sec)/4(count)=5 seconds, the
timer interrupt occurs and then hardware monitor information is read.
The average value of four times' read-in is displayed as a colored
curve.  The width of the "xmbmon" window corresponds to 1800(wsec)
seconds, i.e. 30 minutes.  If time passes old plot data disappears
and renew the display (just like the "xload" program).  The resources
above can be specified as options with the same name at the execution
time.  The help option (-h or -help) shows these options like this:

   # xmbmon -help
X MotherBoard Monitor, ver. 2.02 by YRS.
  options:
    : -g      (100x140) <geometry(Toolkit option)>
    : -count        (4) <counts in an interval>
    : -sec         (20) <seconds of an interval>
    : -wsec      (1800) <total seconds shown>
    : -tmin      (10.0) <min. temperature>
    : -tmax      (50.0) <max. temperature>
    : -vmin      (1.80) <min. voltage>
    : -vmax      (2.20) <max. voltage>
    : -tick         (3) <ticks in ordinate>
    : -cltmb     (blue) <Temp1 color>
    : -cltcpu     (red) <Temp2 color>
    : -cltcs     (cyan) <Temp3 color>
    : -clvc     (green) <Vcore color>
    : -cmtmb      ( MB) <comment of Temp1> [Not shown  ]
    : -cmtcpu     (CPU) <comment of Temp2> [if "NOTEMP"]
    : -cmtcs     (chip) <comment of Temp3> [set.       ]
    : -cmvc       (Vc ) <comment of Vcore>
    : -fahrn    (False) <temp. in Fahrenheit (True|False)>
    : -label        ( ) for showing label [No label if null-string.]
                         and -labelfont, -labelcolor
    : -method       ( ) <access method (V|S|I|A)>
   [: -smbdev [0-9] (0) for using /dev/smb[0-9]<== compiled with -DSMBUS_IOCTL]
    : -extratemp    (2) set extra-temp. to temp[0|1|2] (need -A)
    : -probe chip   ( ) chip=winbond|wl784|via686|it87|lm80|lm90|lm75
                         for probing monitor chips
    : -TyanTigerMP      for Tyan Tiger MP motherboard
    : -debug/DEBUG      for debug information


[Note] From version 2.00 the special treatment of Tyan's dual CPU
  motherboard, TigerMP/MPX is supported.  This motherboard has two
  sensor chips, and one has to enable the second one.  In order to
  attach and enable the second sensor, one has to use a Tyan specific
  option(-Y for "mbmon", -TyanTigerMP for "xmbmon").  Then the first
  sensor is accessed through SMBus and the second one through ISA IO.
  Here the values are the followings:

   1st sensor (SMBus, W83782D)

   temp0      VRM2 temperature
   temp1      CPU1 temperature
   temp2      CPU2 temperature
   Vcore0     CPU1 Vcore
   Vcore1     CPU2 Vcore
   Volt 0     AGP voltage
   Volt 1     system 5V
   Volt 2     DDR voltage
   Volt 3     -----
   Volt 4     standby 3.3V

   2nd sensor (ISA IO, W83627HF)

   temp0      VRM1 temperature
   temp1      AGP  temperature
   temp2      DDR  temperature
   Vcore0     CPU1 Vcore
   Vcore1     CPU2 Vcore
   Volt 0     system 3.3V
   Volt 1     system 5V
   Volt 2     system 12V
   Volt 3     system -12V
   Volt 4     -----



<< 7. Bugs >>

  A serious bug of version prior to 1.06, i.e. "xmbmon" goes out of
control while other processes keep opening /dev/io device file, is
fixed in version 1.06pl1 (patch level 1).  I hope there are no more
serious bugs (but maybe there are!).

  However, among the reports I have received until now, it should be
noticed that there are some motherboards where the hardware monitor
chips are not equipped in accordance with the data sheet instruction.
In such a case the program cannot read the data or the values read by
it does not coincide with those shown in the BIOS.  In such cases,
it is not possible to make the program work properly as long as the
specific information from the motherboard vendor is not provided.


<< 8. Those who want to support other hardware monitor chips >>

  One of the sales point of the present version is that the program
codes are almost completely rewritten in order to make it easier to
support new hardware monitor chips.  This is done by dividing a part
of program into modules (they are "sens_XXXX.c"), each of which
corresponds to each monitor chip that requires different treatments
to read data.  Also, the methods to access SMBus provided by various
Power Management Controllers in the chipsets are different and the
corresponding codes are divided into modules (they are "smbus_XXXX.c").
However, I have just prepared and do not intend to support new monitor
chips and/or other Power Management Controllers which are not supported
in this version.  If there is a person who has a hardware not supported
yet and eagerly want to use it, I hope he or she try to hack the code
and include the support code into the present version
(see 00READMEtech.txt for a brief explanation).

  For such a hacker who wants to develop a support program of new
monitor chips, the first step is to know which device is connected to
the basic PCI bus.  One has to use the FreeBSD command "pciconf".
And in the present package, I have included some test programs for
checking the PCI Configuration, "testpci.c", for checking SMBus Power
Management Controller, "testsmb.c", and for checking the registers to
read the data of Winbond-like monitor chips, "testhwm.c".  For those
who want to support other hardware monitor chips, I encourage them by
providing these small tools.  But don't forget to prepare unexpected
accidents like OS crash, because these tools are more dangerous than
"mbmon/xmbmon" themselves! (Actually, I have destroyed the file system
once, by mis-opened the disk controller and write directly to the
disk!).

  One may be interested in which motherboard uses what kind of
hardware monitor chips.  Such information is available at the homepage
of Alex van Kaam, who is a developer of a famous MBM (Mother Board
Monitor) program on Microsoft Windows platforms,

   http://mbm.livewiredev.com/

Or, in Japan, there is a useful information posted in the mailing-list
run by :p araffin.(Yoneya), who is a developer of other famous
monitoring software LM78mon on Windows platforms,

   http://homepage1.nifty.com/paraffin/

The technical information for Winbond chips are available as PDF files
at Winbond homepage:

   http://www.winbond.com.tw/

And there are many technical informations in the PDF forms are
collected at the hardware monitor (lm_sensor) homepage for Linux at

   http://www.lm-sensors.nu/

  But the information on particular hardware monitor chip may not be
necessarily available, then it is very difficult to support such a chip.


<< 9. Acknowledgments >>

 * The first version opened to the public is ver.1.04, which only
  supports ISA-IO port access method for Winbond chips.  The information
  on actual access method to the chip was opened in the homepage of MBM
  by Alex van Kaam mentioned above (unfortunately, the information is
  not opened now).  If I cannot find this information the programs
  "mbmon/xmbmon" would not appear.  I appreciate that he opened this
  valuable information (which is now available as data sheet PDF files
  at the Winbond homepage now, as shown above).

 * For the next version 1.05, I got a great help by Takanori Watanabe
  at Kobe University, who is a developer of "intpm" driver and
  contributed a lot to support the SMBus ioctl access method.  I have
  included other direct SMBus access method in the version 1.05, the
  SMBus ioctl method is still useful and even safer if there is a
  proper support of OS device drivers (like "intpm" or "viapm").  The
  separation of code for each access method is provided by him, and
  it is a great help for developing the following new version.  I
  appreciate his help.  The (partial) support for Linux has been done
  from ver.1.05 with the help of Koji Okamoto at Kyushu University.
  The information of ASUS AS99127F hardware monitor chip supported from
  ver.1.05 was provided by Noriyuki Yoneya (:p araffin.).  I also
  appreciate their help.

 * In the next version 1.06, I wanted to use "mbmon/xmbmon" for the
  Athlon machines, the number of which are increasing around me because
  of it's fast numeric computing ability.  Here all the Athlon machines
  I can use employ VIA's VT82C686A/B chipset, and it's information
  as a hardware monitor chip was not available.  I got the information
  from, again, Noriyuki Yoneya (:p araffin.).  Furthermore, I found
  that the ASUS motherboard uses their own monitor chip AS99127F,
  even though it employs VT82C686 south chipset.  Then, (since I could
  not use "viapm" device driver on my old FreeBSD-3.X system at that
  time) I need to access the SMBus directly without any OS device driver.
  For this purpose, detailed technical information on the method for
  getting the PCI device configurations, and further, for accessing the
  SMBus directly, are totally provided by Noriyuki Yoneya (:p araffin.).
  Without his help, this version cannot emerge in the present form.
  I appreciate very much his generosity of helping me on this matter.

 * In the next version 1.07, I supported the more powerful facility of
  output in "mbmon" by considering the request of freely utilizing
  the output of the hardware monitor chip.  For this purpose, I
  just included the patch provided by Koji MORITA (morita@cybird.co.jp).
  Thanks a lot for it.

 * The extension of the supporting chips of both the hardware monitor
  and SMBus interface in the version 1.07 has been done thanks to the
  information learned from the source codes of "lm_sensors-2.6.2",
  which is a product of the lm_sensors Project for the Linux platform.
  I greatly appreciate The Lm_sensors Group (http://www.lm-sensors.nu/)
  for developing and opening such useful source codes.
  
 * Until the previous version, the Linux users could not enjoy full
  functions of strong output provided by "mbmon", because some C functions
  are missing.  Michael Lenzen kindly provided equivalent C functions and
  now it is possible to use full functions and the program can be executed
  in the same way in the Linux platform as in FreeBSD form version 2.00.
  I appreciate his help for that.

 * Dramatic changes of the code in the version 2.00 is motivated by the
  contribution of a LM80 chip support code and encouragements given by
  Shin-ichi Nagamura.  Since it is not considered until the previous
  version that new hardware monitor chips will be further supported,
  it was very difficult to include the LM80 code.  Shin-ichi Nagamura
  provided an good example to include a support code corresponding to
  each monitor chip as a module.  Thus, the development of the present
  version started to rewrite the program to divide its part into modules
  for each monitor chip.  I don't think this version would not appear
  if his contribution for separating the program into modules.
  Thanks a lot for this great contributions.

 * In verison 2.00, I have included the support of SMBus accesses
  methods for AMD756/766/768 and ALi M1535D+, which have been long
  desired.  For these support I asked many things again to Noriyuki
  Yoneya (:p araffin.).  Many thanks for giving me adequate advice.

 * And, again, the lm_sensor project, its present newest release version
  is 2.7.0, is always a nice teacher for learning about various monitor
  chips and SMBuses.  Thanks a lot for its wonderful works.

 * As for the special treatment to enable the second sensor chip and
  to initialize the temperature sensors for the Tyan's TigerMP/MPX
  motherboard, Alex van Kaam (creator of MBM) and Tamas Miklos kindly
  gave me the information.  I appreciate their help.

 * For version 2.00, I called for some beta testers: Tom Dean, Julian
  Elischer, Kou Fujimoto, Kazushi Nakamura.  Thank a lot for testing.

 * In this version 2.01, the patch for executing "mbmon" as a daemon mode 
  is a contribution by Jean-Marc Zucconi (jmz@FreeBSD.org).  I believe
  it is useful for somebody monitoring many machines.  Thanks a lot for
  the useful contribution.

 * The improved temperature readout of the ASB100 sensor chip, and the
  support of ASUS Mozart-2 sensor chip are based on the information
  given by the MBM  creator, Alex van Kaam.  Thanks a lot for teaching
  again this time.

 * Fixing the issue of "xmbmon"'s NFS ~/.Xauthority file by a patch
  from Kazushi NAKAMURA (kaz@kobe1995.net).  Thank you.

 * The information of a new SMBus access method of AMD8111 and NVidia
  nForce2 chipsets is given by Alex van Kaam.  Thanks Alex for all
  the helps every times.


<< 10. Finally >>

  The programs are completely free software.   Any modifications and
changes to the programs are welcome, and they are freely copied and
distributed.  The author is not at all responsible if anyone have
any problems or damages when using the programs.  The author won't
keep the development of the programs any further, at least now.  But
if someone develops any new better programs based on my own, I am
very glad if feedback is made to me (of course, it is not at all
any duties!).

   July 31, 2003     Yoshifumi R. Shimizu,
                        Department of Physics,
                        Kyushu University

   e-mail : yrsh2scp@mbox.nc.kyushu-u.ac.jp
   http://www.nt.phys.kyushu-u.ac.jp/shimizu/index.html
