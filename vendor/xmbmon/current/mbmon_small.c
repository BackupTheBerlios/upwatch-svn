/*
 * mbmon  --- command-line motherboard monitor
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mbmon.h"


int main(int argc, char *argv[])
{
	int n, sec;
	float temp1 = 0.0, temp2 = 0.0, temp3 = 0.0;
	float vc0, vc1, v33, v50p, v12p, v12n, v50n;
	int rot1, rot2, rot3;
	extern char *optarg;
	extern int optind;
	char *name;
	int ch, method = ' ';

	name = argv[0];
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
	while ((ch = getopt(argc,argv,"VSIAfdDYe:p:s:")) != -1) {
#else
	while ((ch = getopt(argc,argv,"VSIAfdDYe:p:")) != -1) {
#endif
	  switch(ch) {
	  case 'V':
	    method = 'V';
	    break;
	  case 'S':
	    method = 'S';
	    break;
	  case 'I':
	    method = 'I';
	    break;
	  case 'A':
	    method = 'A';
	    break;
	  case 'f':
		fahrn_flag = 1;
		break;
	  case 'd':
		debug_flag = 1;
	    break;
	  case 'D':
		debug_flag = 2;	/* debug option level 2 */
		break;
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
	  case 's':
		n = atoi(optarg);
		if (n < 0 || n > 9)
			goto exit;
		if (n)
			smb_devbuf[8] = *optarg;
		break;
#endif
	  case 'e':
		n = atoi(optarg);
		if (0 <= n && n <= 2)
			extra_tempNO = n;
		break;
	  case 'p':
		probe_request = optarg;
		break;
	  case 'Y':
		TyanTigerMP_flag = 1;
		break;
	  default:
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
exit:
#endif
		fprintf(stderr, "Usage: %s  <seconds for sleep> (default %d sec)\n"
	" options: [-V|S|I|A (access method)] [-d/D (debug) -f (Fahrenheit)]\n"
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
	"        : [-s [0-9]: for using /dev/smb[0-9]]\n"
#endif
	"        : [-e [0-2] set extra temp. to temp.[0|1|2] (need -A).\n"
	"        : [-p chip (chip=" CHIPLIST " for probing chips)]\n"
	"        : [-Y for Tyan Tiger MP/MPX motherboard]\n"
			, argv[0], DEFAULT_SEC);
	    exit(1);
	  }
	} 
	argc -= optind;
	argv += optind;
	if (argc >= 1) {
	  if ((n = atoi(argv[0])) > 0) {
		sec = n;
	  } else {
	    fprintf(stderr, "  argument (%s) should be integer, stop!\n", argv[0]);
		exit (1);
	  }
	} else {
		sec = DEFAULT_SEC;
	}
	if ((n = InitMBInfo(method)) != 0) {
		perror("InitMBInfo");
		if (n < 0)
			fprintf(stderr,"This program needs \"setuid root\"!!\n");
		exit (1);
	}
	if (debug_flag)
		exit (0);

	while(1) {

/* Temperature */

	getTemp(&temp1, &temp2, &temp3);

	printf("Temp.= %4.1f, %4.1f, %4.1f;",temp1, temp2, temp3);

/* Fan Speeds */

	getFanSp(&rot1, &rot2, &rot3);

	printf(" Rot.= %4d, %4d, %4d\n", rot1, rot2, rot3);

/* Voltages */

	getVolt(&vc0, &vc1, &v33, &v50p, &v50n, &v12p, &v12n);

	printf(" Vcore = %4.2f, %4.2f; Volt. = %4.2f, %4.2f, %5.2f, %6.2f, %5.2f\n", vc0, vc1, v33, v50p, v12p, v12n, v50n);

/* sleep */
	sleep(sec);

	}

}
