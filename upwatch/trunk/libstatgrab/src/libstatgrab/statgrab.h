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

#include <sys/types.h>
#ifdef NETBSD
#include <limits.h>
#endif
#ifdef CYGWIN
#include <sys/unistd.h>
#endif

typedef struct{
        long long user;
        long long kernel;
        long long idle;
        long long iowait;
        long long swap;
        long long nice;
        long long total;
        time_t systime;
}cpu_states_t;

typedef struct{
        float user;
        float kernel;
        float idle;
        float iowait;
        float swap;
        float nice;
	time_t time_taken;
}cpu_percent_t;

typedef struct{
	long long total;
	long long free;
	long long used;
	long long cache;
}mem_stat_t;

typedef struct{
	double min1;
	double min5;
	double min15;
}load_stat_t;

#ifdef SOLARIS
#define MAX_LOGIN_NAME_SIZE 8
#endif
#if defined(LINUX) || defined(FREEBSD)
#define MAX_LOGIN_NAME_SIZE UT_NAMESIZE
#endif
#ifdef NETBSD
#define MAX_LOGIN_NAME_SIZE _POSIX_LOGIN_NAME_MAX
#endif
#if defined(CYGWIN)
#define MAX_LOGIN_NAME_SIZE _SC_LOGIN_NAME_MAX
#endif

typedef struct{
	char *name_list;
	int num_entries;
}user_stat_t;

typedef struct{
	long long total;
	long long used;
	long long free;
}swap_stat_t;

typedef struct{
	char *os_name;
	char *os_release;
	char *os_version;
	char *platform;
	char *hostname;
	time_t uptime;
}general_stat_t;

typedef struct {
        char *device_name;
	char *fs_type;
        char *mnt_point;
        long long size;
        long long used;
        long long avail;
        long long total_inodes;
	long long used_inodes;
        long long free_inodes;
}disk_stat_t;

typedef struct{
	char *disk_name;
	long long read_bytes;
	long long write_bytes;
	time_t systime;
}diskio_stat_t;

typedef struct{
	int total;
	int running;
	int sleeping;
	int stopped;
	int zombie;
}process_stat_t;

typedef struct{
	char *interface_name;
	long long tx;
	long long rx;
	time_t systime;
}network_stat_t;

typedef struct{
	long long pages_pagein;
	long long pages_pageout;
	time_t systime;
}page_stat_t;

cpu_states_t *get_cpu_totals();
cpu_states_t *get_cpu_diff();
cpu_percent_t *cpu_percent_usage();

mem_stat_t *get_memory_stats();

load_stat_t *get_load_stats();

user_stat_t *get_user_stats();

swap_stat_t *get_swap_stats();

general_stat_t *get_general_stats();

disk_stat_t *get_disk_stats(int *entries);
diskio_stat_t *get_diskio_stats(int *entries);
diskio_stat_t *get_diskio_stats_diff(int *entries);

process_stat_t *get_process_stats();

network_stat_t *get_network_stats(int *entries);
network_stat_t *get_network_stats_diff(int *entries);

page_stat_t *get_page_stats();
page_stat_t *get_page_stats_diff();

int statgrab_init(void);