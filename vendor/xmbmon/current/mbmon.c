/*
 *  mbmon  --- command-line motherboard monitor
 *
 */

#ifdef SMALL_MBMON	/* small_mbmon */

#include "mbmon_small.c"

#else			/* ! small_mbmon */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAVE_GETUTENT
#  include <utmp.h>
#else
#  include <sys/sysctl.h>
#endif

#include "mbmon.h"

static const char *MyName = "mbmon";
int port = 0;

int usage(void)
{
	fprintf(stderr, "MotherBoard Monitor, ver. %s by YRS.\n"
"Usage: %s [options...] <seconds for sleep> (default %d sec)\n"
" options:\n"
"  -V|S|I: access method (using \"VIA686 HWM directly\"|\"SMBus\"|\"ISA I/O port\")\n"
"  -A: for probing all methods, all chips, and setting an extra sensor.\n"
"  -d/D: debug mode (any other options except (V|S|I) will be ignored)\n"
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
"  -s [0-9]: for using /dev/smb[0-9]\n"
#endif
"  -e [0-2]: set extra temperature sensor to temp.[0|1|2] (need -A).\n"
"  -p chip: chip="CHIPLIST"\n"
"            for probing chips\n"
"  -Y: for Tyan Tiger MP/MPX motherboard\n"
"  -h: print help message(this) and exit\n"
"  -f: temperature in Fahrenheit\n"
"  -c count: repeat <count> times and exit\n"
"  -P port: run in daemon mode, using given port for clients\n"
"  -T|F [1-7]: print Temperature|Fanspeed according to following styles\n"
"	style1: data1\\n\n"
"	style2: data2\\n\n"
"	style3: data3\\n\n"
"	style4: data1\\ndata2\\n\n"
"	style5: data1\\ndata3\\n\n"
"	style6: data2\\ndata3\\n\n"
"	style7: data1\\ndata2\\ndata3\\n\n"
"  -r: print TAG and Value format\n"
"  -u: print system uptime\n"
"  -t: print present time\n"
"  -n|N: print hostname(long|short style)\n"
"  -i: print integers in the summary(with -T option)\n"\
		, XMBMON_VERSION, MyName, DEFAULT_SEC);
	exit(1);
}

void uptime(time_t now)
{
	int days, hrs, mins, secs;
	struct tm *local_time;
	time_t uptime = 0;

#ifdef LINUX
#define PROC_LOADAVG "/proc/loadavg"

	char file_buf[1024];
	double avg[3];
	int fd;
	ssize_t bytes;
#endif

#ifdef HAVE_GETUTENT
	int nusers = 0;
	struct utmp *utent;

	setutent();
	while ( (utent = getutent()) ) {
		switch (utent->ut_type) {
		case BOOT_TIME:
			uptime = now - utent->ut_time;
			break;

		case USER_PROCESS:
			nusers++;
			break;

		default:
			break;
		}
	}
	endutent();

#else /* FreeBSD, NetBSD */
	struct timeval  boottime;
	int mib[2];
	size_t size;

	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	size = sizeof(boottime);
	if (sysctl(mib, 2, &boottime, &size, NULL, 0) != -1 &&
		boottime.tv_sec != 0)
		uptime = now - boottime.tv_sec;
#endif

        /* display current time */

	local_time = localtime(&now);

	printf("%2d:%02d%s  ",
		local_time->tm_hour%12 ? local_time->tm_hour%12 : 12,
	        local_time->tm_min, local_time->tm_hour > 11 ? "pm" : "am");

        /* display uptime */

	printf("up");
	days = uptime / 86400;
	uptime %= 86400;
	hrs = uptime / 3600;
	uptime %= 3600;
	mins = uptime / 60;
	secs = uptime % 60;
	if (days > 0)
		printf(" %d day%s,", days, days > 1 ? "s" : "");
	if (hrs > 0 && mins > 0)
		printf(" %2d:%02d", hrs, mins);
	else if (hrs > 0)
		printf(" %d hr%s", hrs, hrs > 1 ? "s" : "");
	else if (mins > 0)
		printf(" %d min%s", mins, mins > 1 ? "s" : "");
	else
		printf(" %d sec%s", secs, secs > 1 ? "s" : "");

#ifdef HAVE_GETUTENT
	/* display nusers */

    	printf(", %2d user%s",
		nusers, nusers == 1 ? "" : "s");
#endif

#ifdef LINUX
	/* display load average */

	if ((fd = open(PROC_LOADAVG, O_RDONLY)) == -1) {
                fprintf(stderr, "Error: /proc file system must be mounted\n");
		close(fd);
		exit(1);
	}
	lseek(fd, 0L, SEEK_SET);
	if ((bytes = read(fd, file_buf, sizeof file_buf - 1)) < 0) {
                fprintf(stderr, "Error: can't read file \"%s\"\n",
			PROC_LOADAVG);
		close(fd);
		exit(1);
	}
	file_buf[bytes] = '\0';
	close(fd);

     	if (sscanf(file_buf, "%lf %lf %lf", &avg[0], &avg[1], &avg[2]) < 3) {
		fprintf(stderr, "bad data in file \"%s\"\n", PROC_LOADAVG);
        	exit(1);
	}

    	printf(",  load average: %.2f, %.2f, %.2f", avg[0], avg[1], avg[2]);
#endif

	printf("\n");
}

void hostname(int sh_flag)
{
#ifdef LINUX
	char domainname[MAXHOSTNAMELEN], hostname[MAXHOSTNAMELEN];

	if (gethostname(hostname, (int)sizeof(hostname))) {
		fprintf(stderr, "%s: gethostname failed.\n", MyName);
		perror(MyName);
		exit(1);
	}
	if (getdomainname(domainname, (int)sizeof(domainname))) {
		fprintf(stderr, "%s: getdomainname failed.\n", MyName);
		perror(MyName);
		exit(1);
	}
	if (sh_flag)
	  printf("%s\n", hostname);
	else
	  printf("%s.%s\n", hostname, domainname);
#else
	char *p, hostname[MAXHOSTNAMELEN];

	if (gethostname(hostname, (int)sizeof(hostname))) {
		fprintf(stderr, "%s: gethostname failed.\n", MyName);
		perror(MyName);
		exit(1);
	}
	if (sh_flag && (p = strchr(hostname, '.')))
		*p = '\0';
	printf("%s\n", hostname);
#endif
}


static void daemonize()
{
	int fd;
	struct sigaction sa_ign, sa_save;

#ifdef SIGTSTP
	signal(SIGTSTP, SIG_IGN);
#endif
#ifdef SIGTTIN
	signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTTOU
	signal(SIGTTOU, SIG_IGN);
#endif

	memset(&sa_ign, 0, sizeof(sa_ign));
	sa_ign.sa_handler = SIG_IGN;
	sa_ign.sa_flags = 0;
	sigaction(SIGHUP, &sa_ign, &sa_save);

	switch (fork()) {
	case 0:
		break;

	case -1:
		fprintf(stderr, "%s: daemonize -- fork failed.", MyName);
		perror(MyName);
		exit(1);
		/* NOTREACHED */
		break;

	default:
		exit(0);
		/* NOTREACHED */
		break;
	}

	setsid();

	sigaction(SIGHUP, &sa_save, (struct sigaction *)0);

	if ((fd = open("/dev/null", O_RDWR)) == -1) {
		fprintf(stderr, "%s: daemonize -- open failed.", MyName);
		perror(MyName);
		exit(1);
	}

	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);

	if (fd > 2)
		close (fd);
}


int main(int argc, char *argv[])
{
	int n, sec;
	int count = 0, temperature = 0, fanspeed = 0;
	float temp1 = 0.0, temp2 = 0.0, temp3 = 0.0;
	float vc0, vc1, v33, v50p, v12p, v12n, v50n;
	int rot1, rot2, rot3;
	extern char *optarg;
	extern int optind;
	char *name;
	int fd = 0;
	struct sockaddr_in server, client;
	FILE *out = stdout;

	char buf[256];
	int ch, method = ' ';
	struct tm lt;
	time_t now;
	int time_flag = 0;
	int uptime_flag = 0;
	int hostname_flag = 0;
	int sh_flag = 0;
	int integer_flag = 0;
	int tagval_flag	= 0 ;

#ifdef LOGGING
	int nfd = 0;
	fd_set mstfdset, rd_fdset;
	logdata log_data[LOGENTRIES];
	logdata *curdata = &log_data[0];
	int index = 0;
#endif

	name = argv[0];
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
	while ((ch = getopt(argc,argv,"VSIAfdDYe:p:s:c:T:F:tunNirh")) != -1) {
#else
	while ((ch = getopt(argc,argv,"VSIAfdDYe:p:c:T:F:tunNirhP:")) != -1) {
#endif
	  switch(ch) {
	  case 'V':
		if (method == ' ')
			method = 'V';
		else
			usage();
		break;
	  case 'S':
		if (method == ' ')
			method = 'S';
		else
			usage();
		break;
	  case 'I':
		if (method == ' ')
			method = 'I';
		else
			usage();
		break;
	  case 'A':
		if (method == ' ')
			method = 'A';
		else
			usage();
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
			usage();
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
	  case 'P':
		port = atoi (optarg);
		break;
	  case 'Y':
		TyanTigerMP_flag = 1;
		break;
	  case 'c':
		count = atoi(optarg);
		break;
	  case 'T':
		if (fanspeed != 0)
			usage();
		temperature = atoi(optarg);
		break;
	  case 'F':
		if (temperature != 0)
			usage();
		fanspeed = atoi(optarg);
		break;
	  case 't':
		time_flag = 1;
		break;
	  case 'u':
		uptime_flag = 1;
		break;
	  case 'n':
		if (hostname_flag != 0)
			usage();
		hostname_flag = 1;
		break;
	  case 'N':
		if (hostname_flag != 0)
			usage();
		hostname_flag = 1;
		sh_flag = 1;
		break;
	  case 'i':
		integer_flag = 1;
		break;
	  case 'r':
		tagval_flag = 1;
		break;
	  case 'h':
		usage();
		break;
	  default:
		usage();
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
	
	if (port) {
#ifdef LOGGING
		FILE *log;
		log = fopen(LOGFILE, "w");
		if (log == NULL) {
			perror("fopen(LOGFILE)");
			exit (1);
		}
		fclose(log);
#endif
		fd = socket (AF_INET, SOCK_STREAM, 0);
		if (fd < 0) {
			perror("socket");
			exit (1);
		}
		server.sin_family = AF_INET;
		server.sin_port = htons (port);
		server.sin_addr.s_addr = INADDR_ANY;
		if (bind (fd, (struct sockaddr *) &server, sizeof (server)) < 0) {
			perror("bind");
			exit (1);
		}
		if (listen (fd ,5) < 0) {
			perror("listen");
			exit (1);
		}
		daemonize();
#ifdef LOGGING
		/* clear out all entries for logging */
		FD_ZERO(&mstfdset);
		FD_SET(fd, &mstfdset);
		for (index = 0; index < LOGENTRIES; index++) {
			logdata *ld;
			ld = &log_data[index];
			ld->data = 0;
			ld->prev = &log_data[index-1];
			ld->next = &log_data[index+1];
			if (index == 0) {
				ld->prev = &log_data[LOGENTRIES-1];
			}
			if (index == LOGENTRIES-1) {
				ld->next = &log_data[0];
			}
		}
#endif
	}
	while(1) {

	if (port) {
		int c, len = sizeof (struct sockaddr_in);
#ifdef LOGGING
		struct timeval select_timeout = {LOGINTERVAL, 0};
		memcpy(&rd_fdset, &mstfdset, sizeof(fd_set));
		nfd = select(fd+1, (fd_set *)(&rd_fdset), NULL, NULL, &select_timeout);

		/* open socket as our fd if we accepted a new connection */
		if (nfd > 0) {
#endif
		c = accept (fd, (struct sockaddr *) &client, &len);
		if (c < 0)
			continue;
		out = fdopen (c, "w");
#ifdef LOGGING
		}
#endif
	}
/* get temperature */

	getTemp(&temp1, &temp2, &temp3);

/* get fan speeds */

	getFanSp(&rot1, &rot2, &rot3);

/* get voltages */

	getVolt(&vc0, &vc1, &v33, &v50p, &v50n, &v12p, &v12n);

#ifdef LOGGING
	/* log data */
	if (port && !nfd) {
		time_t tim = time(NULL);
		struct tm *tm = localtime(&tim);
		char timebuf[24];
		char *when;
		FILE *log;
		logdata *prtdata = 0;

		if (tm->tm_hour > 11) {
			when = "PM";
			if (tm->tm_hour > 12)
				tm->tm_hour -= 12;
		} else {
			when = "AM";
			if (tm->tm_hour == 0)
				tm->tm_hour = 12;
		}
			 
		sprintf(timebuf, "%2d/%2d/%4d;%2d:%02d:%02d %s",
			tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900,
			tm->tm_hour, tm->tm_min, tm->tm_sec, when);
		/* format of file is: */
	/* ;;;CPU;Power;Case;CPU;Power; */
	/* 7/10/2003;8:24:08 PM;2015 MHz;44 C;43 C;44 C;2220 RPM;5357 RPM; */
		/* open (purge) log file here */
		log = fopen(LOGFILE, "w");

		/* move new data into first entry */
		curdata = curdata->prev;
		if (!curdata->data)
			curdata->data = malloc(80);
		sprintf(curdata->data,
			"%s;;%5.1f C;%5.1f C;%5.1f C;%5d RPM;%5d RPM;%5d RPM;\n",
			timebuf, temp1, temp2, temp3,
			rot1, rot2, rot3);

		/* rewrite entire file */	
		fprintf(log, ";;;TEMP0;TEMP1;TEMP2;FAN0;FAN1;FAN2;\n");
		for (index = 0, prtdata = curdata;
			index < LOGENTRIES;
			index++, prtdata = prtdata->next) {
			if (!prtdata->data)
				break;
			fprintf(log, prtdata->data);
		}

		/* close log file */
			fclose(log);
	} else
#endif

/* print data */

 	if (tagval_flag) {
 		fprintf(out,  "TEMP0 : %4.1f\n", temp1 ) ;
 		fprintf(out,  "TEMP1 : %4.1f\n", temp2 ) ;
 		fprintf(out,  "TEMP2 : %4.1f\n", temp3 ) ;
 		fprintf(out,  "FAN0  : %4d\n", rot1 ) ;
 		fprintf(out,  "FAN1  : %4d\n", rot2 ) ;
 		fprintf(out,  "FAN2  : %4d\n", rot3 ) ;
 		fprintf(out,  "VC0   : %+6.2f\n", vc0 ) ;
 		fprintf(out,  "VC1   : %+6.2f\n", vc1 ) ;
 		fprintf(out,  "V33   : %+6.2f\n", v33 ) ;
 		fprintf(out,  "V50P  : %+6.2f\n", v50p ) ;
 		fprintf(out,  "V12P  : %+6.2f\n", v12p ) ;
 		fprintf(out,  "V12N  : %+6.2f\n", v12n ) ;
 		fprintf(out,  "V50N  : %+6.2f\n", v50n ) ;
 	} else if (temperature == 0 && fanspeed == 0) {
		fprintf(out, "\n");
		fprintf(out, "Temp.= %4.1f, %4.1f, %4.1f;",temp1, temp2, temp3);
		fprintf(out, " Rot.= %4d, %4d, %4d\n", rot1, rot2, rot3);
		fprintf(out, "Vcore = %4.2f, %4.2f; Volt. = %4.2f, %4.2f, %5.2f, %6.2f, %5.2f\n", vc0, vc1, v33, v50p, v12p, v12n, v50n);
	} else if (fanspeed == 0) {
		if (integer_flag == 0) {
			if (temperature == 1)
				fprintf(out, "%4.1f\n", temp1);
			else if (temperature == 2)
				fprintf(out, "%4.1f\n", temp2);
			else if (temperature == 3)
				fprintf(out, "%4.1f\n", temp3);
			else if (temperature == 4)
				fprintf(out, "%4.1f\n%4.1f\n", temp1, temp2);
			else if (temperature == 5)
				fprintf(out, "%4.1f\n%4.1f\n", temp1, temp3);
			else if (temperature == 6)
				fprintf(out, "%4.1f\n%4.1f\n", temp2, temp3);
			else if (temperature == 7)
				fprintf(out, "%4.1f\n%4.1f\n%4.1f\n", temp1, temp2, temp3);
		} else {
			if (temperature == 1)
				fprintf(out, "%2d\n", (int)(temp1 + 0.5));
			else if (temperature == 2)
				fprintf(out, "%2d\n", (int)(temp2 + 0.5));
			else if (temperature == 3)
				fprintf(out, "%2d\n", (int)(temp3 + 0.5));
			else if (temperature == 4)
				fprintf(out, "%2d\n%2d\n", (int)(temp1 + 0.5), (int)(temp2 + 0.5));
			else if (temperature == 5)
				fprintf(out, "%2d\n%2d\n", (int)(temp1 + 0.5), (int)(temp3 + 0.5));
			else if (temperature == 6)
				fprintf(out, "%2d\n%2d\n", (int)(temp2 + 0.5), (int)(temp3 + 0.5));
			else if (temperature == 7)
				fprintf(out, "%2d\n%2d\n%2d\n", (int)(temp1 + 0.5), (int)(temp2 + 0.5), (int)(temp3 + 0.5));
		}
	} else if (temperature == 0) {
		if (fanspeed == 1)
			fprintf(out, "%4d\n", rot1);
		else if (fanspeed == 2)
			fprintf(out, "%4d\n", rot2);
		else if (fanspeed == 3)
			fprintf(out, "%4d\n", rot3);
		else if (fanspeed == 4)
			fprintf(out, "%4d\n%4d\n", rot1, rot2);
		else if (fanspeed == 5)
			fprintf(out, "%4d\n%4d\n", rot1, rot3);
		else if (fanspeed == 6)
			fprintf(out, "%4d\n%4d\n", rot2, rot3);
		else if (fanspeed == 7)
			fprintf(out, "%4d\n%4d\n%4d\n", rot1, rot2, rot3);
	}
	if (port) {
#ifdef LOGGING
		if (nfd)
#endif
		fclose (out);
		continue;
	}
	now = time(NULL);
	if (time_flag == 1) {
		lt = *localtime(&now);
		strftime(buf, sizeof(buf), "%a %b %e %T %Z %Y", &lt);
		printf("%s\n", buf);
	}
	if (uptime_flag == 1) {
		uptime(now);
	}
	if (hostname_flag == 1) {
		hostname(sh_flag);
	}

/* count */
	if (count != 0) {
	  if (count > 1)
		count--;
	  else
		exit(0);
	}

/* sleep */
	sleep(sec);

	}

}

#endif			/* ! small_mbmon */
