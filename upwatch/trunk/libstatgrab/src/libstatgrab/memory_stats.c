/* 
 * i-scream central monitoring system
 * http://www.i-scream.org
 * Copyright (C) 2000-2003 i-scream
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "statgrab.h"
#include "tools.h"
#ifdef SOLARIS
#include <unistd.h>
#include <kstat.h>
#endif
#if defined(LINUX) || defined(CYGWIN)
#include <stdio.h>
#include <string.h>
#endif
#ifdef FREEBSD
#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>
#endif

mem_stat_t *get_memory_stats(){

	static mem_stat_t mem_stat;

#ifdef SOLARIS
	kstat_ctl_t *kc;
	kstat_t *ksp;
	kstat_named_t *kn;
	long totalmem;
	int pagesize;
#endif
#if defined(LINUX) || defined(CYGWIN)
	char *line_ptr;
	unsigned long long value;
	FILE *f;
#endif
#ifdef FREEBSD
	int mib[2];
	u_long physmem;
	size_t size;
	u_int free_count;
	u_int cache_count;
	u_int inactive_count;
	int pagesize;
#endif
#ifdef NETBSD
	struct uvmexp *uvm;
#endif

#ifdef SOLARIS
	if((pagesize=sysconf(_SC_PAGESIZE)) == -1){
		return NULL;	
	}

	if((totalmem=sysconf(_SC_PHYS_PAGES)) == -1){
		return NULL;
	}

	if ((kc = kstat_open()) == NULL) {
		return NULL;
	}
	if((ksp=kstat_lookup(kc, "unix", 0, "system_pages")) == NULL){
		return NULL;
	}
	if (kstat_read(kc, ksp, 0) == -1) {
		return NULL;
	}
	if((kn=kstat_data_lookup(ksp, "freemem")) == NULL){
		return NULL;
	}
        kstat_close(kc);

	mem_stat.total = (long long)totalmem * (long long)pagesize;
	mem_stat.free = ((long long)kn->value.ul) * (long long)pagesize;
	mem_stat.used = mem_stat.total - mem_stat.free;
#endif

#if defined(LINUX) || defined(CYGWIN)
	if ((f = fopen("/proc/meminfo", "r")) == NULL) {
		return NULL;
	}

	while ((line_ptr = f_read_line(f, "")) != NULL) {
		if (sscanf(line_ptr, "%*s %llu kB", &value) != 1) {
			continue;
		}
		value *= 1024;

		if (strncmp(line_ptr, "MemTotal:", 9) == 0) {
			mem_stat.total = value;
		} else if (strncmp(line_ptr, "MemFree:", 8) == 0) {
			mem_stat.free = value;
		} else if (strncmp(line_ptr, "Cached:", 7) == 0) {
			mem_stat.cache = value;
		}
	}

	fclose(f);
	mem_stat.used = mem_stat.total - mem_stat.free;
#endif

#ifdef FREEBSD
	/* Returns bytes */
	mib[0] = CTL_HW;
	mib[1] = HW_PHYSMEM;
	size = sizeof physmem;
	if (sysctl(mib, 2, &physmem, &size, NULL, 0) < 0) {
		return NULL;
	}
	mem_stat.total = physmem;

	/*returns pages*/
	size = sizeof free_count;
  	if (sysctlbyname("vm.stats.vm.v_free_count", &free_count, &size, NULL, 0) < 0){
		return NULL;
  	}

	size = sizeof inactive_count;
  	if (sysctlbyname("vm.stats.vm.v_inactive_count", &inactive_count , &size, NULL, 0) < 0){
		return NULL;
  	}

	size = sizeof cache_count;
  	if (sysctlbyname("vm.stats.vm.v_cache_count", &cache_count, &size, NULL, 0) < 0){
		return NULL;
  	}

	/* Because all the vm.stats returns pages, I need to get the page size.
 	 * After that I then need to multiple the anything that used vm.stats to
	 * get the system statistics by pagesize 
	 */
	if ((pagesize=getpagesize()) == -1){
		return NULL;
	}

	mem_stat.cache=cache_count*pagesize;

	/* Of couse nothing is ever that simple :) And I have inactive pages to
	 * deal with too. So I'm going to add them to free memory :)
	 */
	mem_stat.free=(free_count*pagesize)+(inactive_count*pagesize);
	mem_stat.used=physmem-mem_stat.free;
#endif

#ifdef NETBSD
	if ((uvm = get_uvmexp()) == NULL) {
		return NULL;
	}
	mem_stat.total = uvm->pagesize * uvm->npages;
	mem_stat.cache = uvm->pagesize * (uvm->filepages + uvm->execpages);
	mem_stat.free = uvm->pagesize * (uvm->free + uvm->inactive);
	mem_stat.used = mem_stat.total - mem_stat.free;
#endif

	return &mem_stat;
}