/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2004  James K. Lowden
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <assert.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include <sqlfront.h>
#include <sybdb.h>

static char software_version[] = "$Id: bsqldb.c,v 1.1.2.1 2004/04/09 22:44:38 jklowden Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr);
int msg_handler(DBPROCESS * dbproc, DBINT msgno, int msgstate, int severity, char *msgtext, 
		char *srvname, char *procname, int line);

static int next_query(DBPROCESS *dbproc);
static void print_results(DBPROCESS *dbproc);
static int get_printable_size(int type, int size);

struct METADATA { char *name, *source, *format_string; int type, size; };
int set_format_string(struct METADATA * meta, const char separator[]);

typedef struct _options 
{ 
	int 	fverbose;
	FILE 	*verbose;
	char 	*servername, 
		*database, 
		*appname, 
		 hostname[128], 
		*input_filename, 
		*output_filename, 
		*error_filename; 
} OPTIONS;

LOGINREC* get_login(int argc, char *argv[], OPTIONS *poptions);

/* global variables */
OPTIONS options;
/* end global variables */


/**
 * The purpose of this program is threefold:
 *
 * 1.  To provide a generalized SQL processor suitable for testing db-lib.
 * 2.  To offer a robust batch-oriented SQL processor suitable for use in a production environment.  
 * 3.  To serve as a model example of how to use db-lib functions.  
 *
 * These purposes may be somewhat at odds with one another.  For instance, the tutorial aspect calls for
 * explanatory comments that wouldn't appear in production code.  Nevertheless, I hope the  experienced
 * reader will forgive the verbosity and still find the program useful.  
 *
 * \todo The error/message handlers are not robust enough.  They should anticipate certain conditions 
 * and cause the application to retry the operation.  
 */
int
main(int argc, char *argv[])
{
	LOGINREC *login;
	DBPROCESS *dbproc;
	RETCODE erc;

	/* Initialize db-lib */
	erc = dbinit();	
	if (erc == FAIL) {
		fprintf(stderr, "%s:%d: dbinit() failed\n", options.appname, __LINE__);
		exit(1);
	}
	

	memset(&options, 0, sizeof(options));
	login = get_login(argc, argv, &options); /* get command-line parameters and call dblogin() */
	assert(login != NULL);

	/* Install our error and message handlers */
	dberrhandle(err_handler);
	dbmsghandle(msg_handler);

	/* 
	 * Override stdin, stdout, and stderr, as required 
	 */
	if (options.input_filename) {
		if (freopen(options.input_filename, "r", stdin) < 0) {
			fprintf(stderr, "%s: unable to open %s: %s\n", options.appname, options.input_filename, strerror(errno));
			exit(1);
		}
	}

	if (options.output_filename) {
		if (freopen(options.output_filename, "w", stdout) < 0) {
			fprintf(stderr, "%s: unable to open %s: %s\n", options.appname, options.output_filename, strerror(errno));
			exit(1);
		}
	}
	
	if (options.error_filename) {
		if (freopen(options.error_filename, "w", stderr) < 0) {
			fprintf(stderr, "%s: unable to open %s: %s\n", options.appname, options.error_filename, strerror(errno));
			exit(1);
		}
	}

	if (options.fverbose) {
		options.verbose = stderr;
	} else {
		static const char null_device[] = "/dev/null";
		options.verbose = fopen(null_device, "w");
		if (options.verbose < 0) {
			fprintf(stderr, "%s:%d unable to open %s for verbose operation: %s\n", 
					options.appname, __LINE__, null_device, strerror(errno));
			exit(1);
		}
	}

	fprintf(options.verbose, "%s:%d: Verbose operation enabled\n", options.appname, __LINE__);
	
	/* 
	 * Connect to the server 
	 */
	dbproc = dbopen(login, options.servername);
	assert(dbproc != NULL);
	
	/* Switch to the specified database, if any */
	if (options.database)
		dbuse(dbproc, options.database);

	/* 
	 * Read the queries and write the results
	 */
	while (next_query(dbproc) != -1 ) {
	
		/* Send the query to the server (we could use dbsqlexec(), instead) */
		erc = dbsqlsend(dbproc);
		if (erc == FAIL) {
			fprintf(stderr, "%s:%d: dbsqlsend() failed\n", options.appname, __LINE__);
			exit(1);
		}
		fprintf(options.verbose, "%s:%d: dbsqlsend() OK:\n", options.appname, __LINE__);
		
		/* Wait for it to execute */
		erc = dbsqlok(dbproc);
		if (erc == FAIL) {
			fprintf(stderr, "%s:%d: dbsqlok() failed\n", options.appname, __LINE__);
			exit(1);
		}
		fprintf(options.verbose, "%s:%d: dbsqlok() OK:\n", options.appname, __LINE__);

		/* Write the output */
		print_results(dbproc);
	}

	return 0;
}

int
next_query(DBPROCESS *dbproc)
{
	static const char go[] = "go\n";
	char query_line[4096];
	RETCODE erc;
	
	if (feof(stdin))
		return -1;
			
	fprintf(options.verbose, "%s:%d: Query:\n", options.appname, __LINE__);
	
	while (fgets(query_line, sizeof(query_line), stdin)) {
		/* 'go' or 'GO' separates command batches */
		if (0 == strcasecmp(query_line, go))
			return 1;
			
		fprintf(options.verbose, "\t%s", query_line);
		
		/* Add the query line to the command to be sent to the server */
		erc = dbcmd(dbproc, query_line);
		if (erc == FAIL) {
			fprintf(stderr, "%s:%d: dbcmd() failed\n", options.appname, __LINE__);
			return -1;
		}
	}
	
	if (feof(stdin))
		return 0;
			
	if (ferror(stdin)) {
		fprintf(stderr, "%s:%d: next_query() failed\n", options.appname, __LINE__);
		perror(NULL);
		return -1;
	}
	
	return 1;
}

void
print_results(DBPROCESS *dbproc) 
{
	static const char empty_string[] = "";
	static const char dashes[] = "----------------------------------------------------------------" /* each line is 64 */
				     "----------------------------------------------------------------"
				     "----------------------------------------------------------------"
				     "----------------------------------------------------------------";
	
	struct METADATA *metadata = NULL, return_status;
	
	struct DATA { char *buffer; int status; } *data = NULL;
	
	struct METACOMP { int numalts; struct METADATA *meta; struct DATA *data; } **metacompute = NULL;
	
	RETCODE erc;
	int row_code;
	int i, c, ret;
	int iresultset;
	int ncomputeids = 0, ncols = 0;
	
	/* 
	 * Set up each result set with dbresults()
	 * This is more commonly implemented as a while() loop, but we're counting the result sets. 
	 */
	fprintf(options.verbose, "%s:%d: calling dbresults OK:\n", options.appname, __LINE__);
	for (iresultset=1; (erc = dbresults(dbproc)) != NO_MORE_RESULTS; iresultset++) {
		if (erc == FAIL) {
			fprintf(stderr, "%s:%d: dbresults(), result set %d failed\n", options.appname, __LINE__, iresultset);
			return;
		}
		
		fprintf(options.verbose, "Result set %d\n", iresultset);
		/* Free prior allocations, if any. */
		fprintf(options.verbose, "Freeing prior allocations\n", iresultset);
		for (c=0; c < ncols; c++) {
			free(metadata[c].format_string);
			free(data[c].buffer);
		}
		free(metadata);
		metadata = NULL;
		free(data);
		data = NULL;
		ncols = 0;
		
		for (i=0; i < ncomputeids; i++) {
			for (c=0; c < metacompute[i]->numalts; c++) {
				free(metacompute[i]->meta[c].name);
				free(metacompute[i]->meta[c].format_string);
			}
			free(metacompute[i]->meta);
			free(metacompute[i]->data);
			free(metacompute[i]);
		}
		free(metacompute);
		metacompute = NULL;
		ncomputeids = 0;
		
		/* 
		 * Allocate memory for metadata and bound columns 
		 */
		fprintf(options.verbose, "Allocating buffers\n", iresultset);
		ncols = dbnumcols(dbproc);	

		metadata = (struct METADATA*) calloc(ncols, sizeof(struct METADATA));
		assert(metadata);

		data = (struct DATA*) calloc(ncols, sizeof(struct DATA));
		assert(data);
		
		/* metadata is more complicated only because there may be several compute ids for each result set */
		fprintf(options.verbose, "Allocating compute buffers\n", iresultset);
		ncomputeids = dbnumcompute(dbproc);
		if (ncomputeids > 0) {
			metacompute = (struct METACOMP**) calloc(ncomputeids, sizeof(struct METACOMP*));
			assert(metacompute);
		}
		
		for (i=0; i < ncomputeids; i++) {
			metacompute[i] = (struct METACOMP*) calloc(ncomputeids, sizeof(struct METACOMP));
			assert(metacompute[i]);
			metacompute[i]->numalts = dbnumalts(dbproc, 1+i);
			fprintf(options.verbose, "%d columns found in computeid %d\n", metacompute[i]->numalts, 1+i);
			if (metacompute[i]->numalts > 0) {
				fprintf(options.verbose, "allocating column %d\n", 1+i);
				metacompute[i]->meta = (struct METADATA*) calloc(metacompute[i]->numalts, sizeof(struct METADATA));
				assert(metacompute[i]->meta);
				metacompute[i]->data = (struct     DATA*) calloc(metacompute[i]->numalts, sizeof(struct     DATA));
				assert(metacompute[i]->data);
			}
		}

		/* 
		 * For each column, get its name, type, and size. 
		 * Allocate a buffer to hold the data, and bind the buffer to the column.
		 * "bind" here means to give db-lib the address of the buffer we want filled as each row is fetched.
		 * TODO: Implement dbcoltypeinfo() for numeric/decimal datatypes.  
		 */

		fprintf(options.verbose, "Metadata\n", iresultset);
		fprintf(options.verbose, "%-6s  %-30s  %-30s  %-15s  %-6s  %-6s  \n", "col", "name", "source", "type", "size", "varys");
		fprintf(options.verbose, "%.6s  %.30s  %.30s  %.15s  %.6s  %.6s  \n", dashes, dashes, dashes, dashes, dashes, dashes);
		for (c=0; c < ncols; c++) {
			int width;
			/* Get and print the metadata.  Optional: get only what you need. */
			char *name = dbcolname(dbproc, c+1);
			metadata[c].name = (name)? name : empty_string;

			name = dbcolsource(dbproc, c+1);
			metadata[c].source = (name)? name : empty_string;

			metadata[c].type = dbcoltype(dbproc, c+1);
			metadata[c].size = dbcollen(dbproc, c+1);
			assert(metadata[c].size != -1); /* -1 means indicates an out-of-range request*/

			fprintf(options.verbose, "%6d  %30s  %30s  %15s  %6d  %6d  \n", 
				c+1, metadata[c].name, metadata[c].source, dbprtype(metadata[c].type), 
				metadata[c].size,  dbvarylen(dbproc, c+1));

			/* 
			 * Build the column header format string, based on the column width. 
			 * This is just one solution to the question, "How wide should my columns be when I print them out?"
			 */
			width = get_printable_size(metadata[c].type, metadata[c].size);
			if (width < strlen(metadata[c].name))
				width = strlen(metadata[c].name);
				
			ret = set_format_string(&metadata[c], (c+1 < ncols)? "  " : "\n");
			if (ret <= 0) {
				fprintf(stderr, "%s:%d: asprintf(), column %d failed\n", options.appname, __LINE__, c+1);
				return;
			}

			/* 
			 * Bind the column to our variable.
			 * We bind everything to strings, because we want db-lib to convert everything to strings for us.
			 * If you're performing calculations on the data in your application, you'd bind the numeric data
			 * to C integers and floats, etc. instead. 
			 * 
			 * It is not necessary to bind to every column returned by the query.  
			 * Data in unbound columns are simply never copied to the user's buffers and are thus 
			 * inaccesible to the application.  
			 */

			data[c].buffer = calloc(1, metadata[c].size);
			assert(data[c].buffer);

			erc = dbbind(dbproc, c+1, STRINGBIND, -1, (BYTE *) data[c].buffer);
			if (erc == FAIL) {
				fprintf(stderr, "%s:%d: dbbind(), column %d failed\n", options.appname, __LINE__, c+1);
				return;
			}

			erc = dbnullbind(dbproc, c+1, &data[c].status);
			if (erc == FAIL) {
				fprintf(stderr, "%s:%d: dbnullbind(), column %d failed\n", options.appname, __LINE__, c+1);
				return;
			}
		}
		
		/* 
		 * Get metadata and bind the columns for any compute rows.
		 */
		for (i=0; i < ncomputeids; i++) {
			fprintf(options.verbose, "For computeid %d:\n", 1+i);
			for (c=0; c < metacompute[i]->numalts; c++) {
				/* read metadata */
				struct METADATA *meta = &metacompute[i]->meta[c];
				int nbylist, ibylist;
				BYTE *bylist;
				char *colname, bynames[256] = "by (";
				int altcolid = dbaltcolid(dbproc, i+1, c+1);
				
				metacompute[i]->meta[c].type = dbalttype(dbproc, i+1, c+1);
				metacompute[i]->meta[c].size = dbaltlen(dbproc, i+1, c+1);

				/* 
				 * Jump through hoops to determine a useful name for the computed column 
				 * If the query says "compute count(c) by a,b", we get a "by list" indicating a & b.  
				 */
				bylist = dbbylist(dbproc, c+1, &nbylist);

				for (ibylist=0; ibylist < nbylist; ibylist++) {
					int ret;
					char *s = strchr(bynames, '\0'); 
					int remaining = bynames + sizeof(bynames) - s;
					assert(remaining > 0);
					ret = snprintf(s, remaining, "%s%s", dbcolname(dbproc, bylist[ibylist]), 
										(ibylist+1 < nbylist)? ", " : ")");
					if (ret <= 0) {
						fprintf(options.verbose, "Insufficient room to create name for column %d:\n", 1+c);
						break;
					}
				}
				
				if( altcolid == -1 ) {
					colname = "*";
				} else {
					colname = metadata[altcolid].name;
				}

				asprintf(&metacompute[i]->meta[c].name, "%s(%s)", dbprtype(dbaltop(dbproc, i+1, c+1)), colname);
				assert(metacompute[i]->meta[c].name);
					
				ret = set_format_string(meta, (c+1 < metacompute[i]->numalts)? "  " : "\n");
				if (ret <= 0) {
					fprintf(stderr, "%s:%d: asprintf(), column %d failed\n", options.appname, __LINE__, c+1);
					return;
				}
				
				fprintf(options.verbose, "\tcolumn %d is %s, type %s, size %d %s\n", 
					c+1, metacompute[i]->meta[c].name, dbprtype(metacompute[i]->meta[c].type), metacompute[i]->meta[c].size, 
					(nbylist > 0)? bynames : "");
	
				/* allocate buffer */
				assert(metacompute[i]->data);
				metacompute[i]->data[c].buffer = calloc(1, metacompute[i]->meta[c].size);
				assert(metacompute[i]->data[c].buffer);
				
				/* bind */
				erc = dbaltbind(dbproc, i+1, c+1, STRINGBIND, -1, metacompute[i]->data[c].buffer);
				if (erc == FAIL) {
					fprintf(stderr, "%s:%d: dbaltbind(), column %d failed\n", options.appname, __LINE__, c+1);
					return;
				}
			}
		}
		
		fprintf(options.verbose, "\n");
		fprintf(options.verbose, "Data\n", iresultset);

		/* Print the column headers to stderr to keep them separate from the data.  */
		for (c=0; c < ncols; c++) {
			char fmt[256] = "%-";
			
			/* left justify the names */
			strcat(fmt, &metadata[c].format_string[1]);
			fprintf(stderr, fmt, metadata[c].name);
		}

		/* Underline the column headers.  */
		for (c=0; c < ncols; c++) {
			fprintf(stderr, metadata[c].format_string, dashes);
		}

		/* 
		 * Print the data to stdout.  
		 */
		while ((row_code = dbnextrow(dbproc)) != NO_MORE_ROWS) {
			switch (row_code) {
			case REG_ROW:
				for (c=0; c < ncols; c++) {
					switch (data[c].status) { /* handle nulls */
					case -1: /* is null */
						/* TODO: FreeTDS 0.62 does not support dbsetnull() */
						fprintf(stdout, metadata[c].format_string, "NULL");
						break;
					case 0:
					/* case >1 is datlen when buffer is too small */
					default:
						fprintf(stdout, metadata[c].format_string, data[c].buffer);
						break;
					}
				}
				break;
				
			case BUF_FULL:
				assert(row_code != BUF_FULL);
				break;
				
			default: /* computeid */
				fprintf(options.verbose, "Data for computeid %d\n", row_code);
				for (c=0; c < metacompute[row_code-1]->numalts; c++) {
					char fmt[256] = "%-";
					struct METADATA *meta = &metacompute[row_code-1]->meta[c];
					
					/* left justify the names */
					strcat(fmt, &meta->format_string[1]);
					fprintf(stderr, fmt, meta->name);
				}

				/* Underline the column headers.  */
				for (c=0; c < metacompute[row_code-1]->numalts; c++) {
					fprintf(stderr, metacompute[row_code-1]->meta[c].format_string, dashes);
				}
					
				for (c=0; c < metacompute[row_code-1]->numalts; c++) {
					struct METADATA *meta = &metacompute[row_code-1]->meta[c];
					struct     DATA *data = &metacompute[row_code-1]->data[c];
					
					switch (data->status) { /* handle nulls */
					case -1: /* is null */
						/* TODO: FreeTDS 0.62 does not support dbsetnull() */
						fprintf(stdout, meta->format_string, "NULL");
						break;
					case 0:
					/* case >1 is datlen when buffer is too small */
					default:
						fprintf(stdout, meta->format_string, data->buffer);
						break;
					}
				}
			}


		}

		/* Check return status */
		fprintf(options.verbose, "Retrieving return status... ");
		if (dbhasretstat(dbproc) == TRUE) {
			fprintf(stderr, "Procedure returned %d\n", dbretstatus(dbproc));
		} else {
			fprintf(options.verbose, "none\n");
		}
		
		/* 
		 * Get row count, if available.   
		 */
		if (DBCOUNT(dbproc) > -1)
			fprintf(stderr, "%d rows affected\n", DBCOUNT(dbproc));
			

		/* 
		 * Check return parameter values 
		 */
		fprintf(options.verbose, "Retrieving output parameters... ");
		if (dbnumrets(dbproc) > 0) {
			for (i = 1; i <= dbnumrets(dbproc); i++) {
				char parameter_string[1024];
				
				return_status.name = dbretname(dbproc, i);
				fprintf(stderr, "ret name %d is %s\n", i, return_status.name);
				
				return_status.type = dbrettype(dbproc, i);
				fprintf(options.verbose, "\n\tret type %d is %d", i, return_status.type);
				
				return_status.size = dbretlen(dbproc, i);
				fprintf(options.verbose, "\n\tret len %d is %d\n", i, return_status.size);
				
				dbconvert(dbproc, return_status.type, dbretdata(dbproc, i), return_status.size, 
					  SYBVARCHAR, (BYTE *) parameter_string, -1);
				fprintf(stderr, "ret data %d is %s\n", i, parameter_string);
			}
		} else {
			fprintf(options.verbose, "none\n");
		}
	} /* wend dbresults */
	fprintf(options.verbose, "%s:%d: dbresults() returned NO_MORE_RESULTS (%d):\n", options.appname, __LINE__, erc);
}

static int
get_printable_size(int type, int size)	/* adapted from src/dblib/dblib.c */
{
	switch (type) {
	case SYBINTN:
		switch (size) {
		case 1:
			return 3;
		case 2:
			return 6;
		case 4:
			return 11;
		case 8:
			return 21;
		}
	case SYBINT1:
		return 3;
	case SYBINT2:
		return 6;
	case SYBINT4:
		return 11;
	case SYBINT8:
		return 21;
	case SYBVARCHAR:
	case SYBCHAR:
		return size;
	case SYBFLT8:
		return 11;	/* FIX ME -- we do not track precision */
	case SYBREAL:
		return 11;	/* FIX ME -- we do not track precision */
	case SYBMONEY:
		return 12;	/* FIX ME */
	case SYBMONEY4:
		return 12;	/* FIX ME */
	case SYBDATETIME:
		return 26;	/* FIX ME */
	case SYBDATETIME4:
		return 26;	/* FIX ME */
#if 0	/* seems not to be exported to sybdb.h */
	case SYBBITN:
#endif
	case SYBBIT:
		return 1;
		/* FIX ME -- not all types present */
	default:
		return 0;
	}

}

/** 
 * Build the column header format string, based on the column width. 
 * This is just one solution to the question, "How wide should my columns be when I print them out?"
 */
int
set_format_string(struct METADATA * meta, const char separator[])
{
	int width, ret;
	assert(meta);
	
	width = get_printable_size(meta->type, meta->size);
	if (width < strlen(meta->name))
		width = strlen(meta->name);

	ret = asprintf(&meta->format_string, "%%%d.%ds%s", width, width, separator);
		       
	return ret;
}

void
usage(const char invoked_as[])
{
	fprintf(stderr, "usage:  %s \n"
			"        [-U username] [-P password]\n"
			"        [-S servername] [-D database]\n"
			"        [-i input filename] [-o output filename] [-e error filename]\n"
			, invoked_as);
}

LOGINREC *
get_login(int argc, char *argv[], OPTIONS *options)
{
	LOGINREC *login;
	int ch;

	extern char *optarg;
	extern int optind;

	assert(options && argv);
	
	options->appname = basename(argv[0]);
	
	login = dblogin();
	
	if (!login) {
		fprintf(stderr, "%s: unable to allocate login structure\n", options->appname);
		exit(1);
	}
	
	DBSETLAPP(login, options->appname);
	
	if (-1 == gethostname(options->hostname, sizeof(options->hostname))) {
		perror("unable to get hostname");
	} else {
		DBSETLHOST(login, options->hostname);
	}

	while ((ch = getopt(argc, argv, "U:P:S:D:i:o:e:v")) != -1) {
		switch (ch) {
		case 'U':
			DBSETLUSER(login, optarg);
			break;
		case 'P':
			DBSETLPWD(login, optarg);
			break;
		case 'S':
			options->servername = strdup(optarg);
			break;
		case 'D':
			options->database = strdup(optarg);
			break;
		case 'i':
			options->input_filename = strdup(optarg);
			break;
		case 'o':
			options->output_filename = strdup(optarg);
			break;
		case 'e':
			options->error_filename = strdup(optarg);
			break;
		case 'v':
			options->fverbose = 1;
			break;
		case '?':
		default:
			usage(options->appname);
			exit(1);
		}
	}
	
	if (!options->servername) {
		usage(options->appname);
		exit(1);
	}
	
	return login;
}

int
err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr)
{

	if (dberr) {
		fprintf(stderr, "%s: Msg %d, Level %d\n", options.appname, dberr, severity);
		fprintf(stderr, "%s\n\n", dberrstr);
	}

	else {
		fprintf(stderr, "%s: DB-LIBRARY error:\n\t", options.appname);
		fprintf(stderr, "%s\n", dberrstr);
	}

	return INT_CANCEL;
}

int
msg_handler(DBPROCESS * dbproc, DBINT msgno, int msgstate, int severity, char *msgtext, char *srvname, char *procname, int line)
{
	printf("Msg %ld, Level %d, State %d\n", (long) msgno, severity, msgstate);

	if (strlen(srvname) > 0)
		printf("Server '%s', ", srvname);
	if (strlen(procname) > 0)
		printf("Procedure '%s', ", procname);
	if (line > 0)
		printf("Line %d", line);

	printf("\n\t%s\n", msgtext);

	return 0;
}
