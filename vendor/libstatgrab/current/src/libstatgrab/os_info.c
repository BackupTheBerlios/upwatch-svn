/*
 * i-scream libstatgrab
 * http://www.i-scream.org
 * Copyright (C) 2000-2004 i-scream
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA
 *
 * $Id: os_info.c,v 1.19 2004/07/18 21:30:11 ats Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/utsname.h>
#include "statgrab.h"
#include <stdlib.h>
#ifdef SOLARIS
#include <kstat.h>
#include <time.h>
#endif
#if defined(LINUX) || defined(CYGWIN)
#include <stdio.h>
#endif
#ifdef ALLBSD
#if defined(FREEBSD) || defined(DFBSD)
#include <sys/types.h>
#include <sys/sysctl.h>
#else
#include <sys/param.h>
#include <sys/sysctl.h>
#endif
#include <time.h>
#include <sys/time.h>
#endif

sg_host_info *sg_get_host_info(){

	static sg_host_info general_stat;	
	static struct utsname os;

#ifdef SOLARIS
	time_t boottime,curtime;
	kstat_ctl_t *kc;
	kstat_t *ksp;
	kstat_named_t *kn;
#endif
#if defined(LINUX) || defined(CYGWIN)
	FILE *f;
#endif
#ifdef ALLBSD
	int mib[2];
	struct timeval boottime;
	time_t curtime;
	size_t size;
#endif

	if((uname(&os)) < 0){
		sg_set_error_with_errno(SG_ERROR_UNAME, NULL);
		return NULL;
	}
	
	general_stat.os_name = os.sysname;
	general_stat.os_release = os.release;
	general_stat.os_version = os.version;
	general_stat.platform = os.machine;
	general_stat.hostname = os.nodename;

	/* get uptime */
#ifdef SOLARIS
	if ((kc = kstat_open()) == NULL) {
		sg_set_error(SG_ERROR_KSTAT_OPEN, NULL);
		return NULL;
	}
	if((ksp=kstat_lookup(kc, "unix", -1, "system_misc"))==NULL){
		sg_set_error(SG_ERROR_KSTAT_LOOKUP, "unix,-1,system_misc");
		return NULL;
	}
	if (kstat_read(kc, ksp, 0) == -1) {
		sg_set_error(SG_ERROR_KSTAT_READ, NULL);
		return NULL;
	}
	if((kn=kstat_data_lookup(ksp, "boot_time")) == NULL){
		sg_set_error(SG_ERROR_KSTAT_DATA_LOOKUP, "boot_time");
		return NULL;
	}
	boottime=(kn->value.ui32);

	kstat_close(kc);

	time(&curtime);
	general_stat.uptime = curtime - boottime;
#endif
#if defined(LINUX) || defined(CYGWIN)
	if ((f=fopen("/proc/uptime", "r")) == NULL) {
		sg_set_error_with_errno(SG_ERROR_OPEN, "/proc/uptime");
		return NULL;
	}
	if((fscanf(f,"%lu %*d",&general_stat.uptime)) != 1){
		sg_set_error(SG_ERROR_PARSE, NULL);
		return NULL;
	}
	fclose(f);
#endif
#ifdef ALLBSD
	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	size = sizeof boottime;
	if (sysctl(mib, 2, &boottime, &size, NULL, 0) < 0){
		sg_set_error_with_errno(SG_ERROR_SYSCTL,
		                        "CTL_KERN.KERN_BOOTTIME");
		return NULL;
	}
	time(&curtime);
	general_stat.uptime=curtime-boottime.tv_sec;
#endif

	return &general_stat;
	
}
