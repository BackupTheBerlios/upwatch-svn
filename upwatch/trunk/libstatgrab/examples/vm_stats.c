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

#include <stdio.h>
#include <statgrab.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv){
	
	extern char *optarg;
        extern int optind;
        int c;

	int delay = 1;
	mem_stat_t *mem_stats;
	swap_stat_t *swap_stats;

	long long total, free;

	while ((c = getopt(argc, argv, "d:")) != -1){
                switch (c){
                        case 'd':
                                delay = atoi(optarg);
                                break;
		}
	}

	if( ((mem_stats=get_memory_stats()) != NULL) && (swap_stats=get_swap_stats()) != NULL){
		printf("Total memory in bytes : %lld\n", mem_stats->total);
		printf("Used memory in bytes : %lld\n", mem_stats->used);
		printf("Cache memory in bytes : %lld\n", mem_stats->cache);
		printf("Free memory in bytes : %lld\n", mem_stats->free);

		printf("Swap total in bytes : %lld\n", swap_stats->total);
		printf("Swap used in bytes : %lld\n", swap_stats->used);	
		printf("Swap free in bytes : %lld\n", swap_stats->free);

		total = mem_stats->total + swap_stats->total;
		free = mem_stats->free + swap_stats->free;

		printf("Total VM usage : %5.2f%%\n", 100 - (((float)total/(float)free)));

	}
	exit(0);
}


