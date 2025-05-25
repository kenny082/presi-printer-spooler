/**
 * === DO NOT MODIFY THIS FILE ===
 * If you need some other prototypes or constants in a header, please put them
 * in another header file.
 *
 * When we grade, we will be replacing this file with our own copy.
 * You have been warned.
 * === DO NOT MODIFY THIS FILE ===
 */
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "presi.h"
#include "conversions.h"

/*
 * "Presi" printer spooler.
 */
int main(int argc, char *argv[])
{
    char optval;
    FILE *in = NULL;
    FILE *out = stdout;
    int quit = 0;

    while(optind < argc) {
	if((optval = getopt(argc, argv, "i:o:qt")) != -1) {
	    switch(optval) {
	    case 'i':
		in = fopen(optarg, "r");
		if(in == NULL) {
		    perror("input file");
		    exit(EXIT_FAILURE);
		}
		break;
	    case 'o':
		if((out = fopen(optarg, "w")) == NULL) {
		    perror("write_file");
		    exit(EXIT_FAILURE);
		}
		break;
	    case 'q':
		// Set to nonzero to suppress event tracing printout to stderr.
		extern int sf_suppress_chatter;
		sf_suppress_chatter = 1;
		break;
	    case 't':
		// Send events to the event tracker for automated testing.
		// Turn off printout when tracking because it is not async-signal-safe and seems
		// to cause hanging in some situations.
		extern int sf_enable_tracking;
		sf_enable_tracking = 1;
//		sf_suppress_chatter = 1;
		break;
	    case '?':
		fprintf(stderr, "Usage: %s [-q][-t][-i <cmd_file>] [-o <out_file>]\n", argv[0]);
		exit(EXIT_FAILURE);
		break;
	    default:
		break;
	    }
	}
    }
    sf_init();
    conversions_init();
    if(in != NULL) {
	quit = run_cli(in, out);
	fclose(in);
	fflush(out);
    }
    if(!quit) {
	run_cli(stdin, out);
	fflush(out);
    }
    conversions_fini();
    sf_fini();
    exit(EXIT_SUCCESS);
}
