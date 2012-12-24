#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>

#include <ownetapi.h>

//------------- Globals vaiables ----------
char *owserver_address = "4304";
char *one_wire_path = "/";
long number_reps = 1000 ;

//------------- Usage information --------
void usage(int argc, char **argv)
{
	printf("%s is an example of the OWNET_process function\n", basename(argv[0]));
	printf("\tevery directory path is traversed recursively\n");
	printf("\n");
	printf("Usage of %s:\n", basename(argv[0]));
	printf("\t%s -s owserver_address one_wire_path\n", argv[0]);
	printf("\t\towserver_address -- tcp/ip address:port of owserver\n");
	printf("\t\t\te.g. 192.168.0.77:3000 or just port number\n");
	printf("\t\t\tdefault localhost:4304\n");
	printf("\t\tone_wire_path    -- OWFS-style 1-wire address.\n");
	printf("\t\t\te.g.: / for root 1-wire bus listing (default)\n");
	printf("\t\t\tor /10.67C6697351FF for a device directory\n");
	printf("\t\t -r number_of_iterations\n");
	printf("\t\t\t default 1000\n");
	printf("\n");
	printf("see http://www.owfs.org for information on owserver.\n");
	exit(1);
}

//------------- Command line parsing -----
void parse_command_line(int argc, char **argv)
{
	int argc_index;
	enum { ni_unknown, ni_owserver, ni_rep, } next_is = ni_unknown ;
	if (argc == 1) {
		usage(argc, argv);
	}
	for (argc_index = 1; argc_index < argc; ++argc_index) {
		if (strcmp(argv[argc_index], "-h") == 0) {
			usage(argc, argv);
			next_is = ni_unknown ;
		} else if (strcmp(argv[argc_index], "--help") == 0) {
			usage(argc, argv);
			next_is = ni_unknown ;
		} else if (strcmp(argv[argc_index], "-s") == 0) {
			next_is = ni_owserver ;
		} else if (strcmp(argv[argc_index], "-r") == 0) {
			next_is = ni_rep ;
		} else if (strcmp(argv[argc_index], "--server") == 0) {
			next_is = ni_owserver ;
		} else {
			switch ( next_is ) {
				case ni_rep:
					number_reps = atol( argv[argc_index] ) ;
					if ( number_reps < 1 || number_reps > 10000000 ) {
						fprintf(stderr,"Repetitions out of range\n");
						exit(1) ;
					}
					break ;
				case ni_owserver:
					owserver_address = argv[argc_index];
					break ;
				case ni_unknown:
				default:
					one_wire_path = argv[argc_index];
					break ;
			}
			next_is = ni_unknown ;
		}
	}
}

//------------- Example-specific ---------
struct pass_on {
	OWNET_HANDLE h;
	int depth;
	long number ;
};

// directory_element callback function
void Count_element(void *v, const char *filename)
{
	// cast the void pointer to a known structure pointer
	struct pass_on *this_pass = v;
	// set up a structure to pass to future call-back
	struct pass_on next_pass = {
		this_pass->h,
		this_pass->depth + 1,
		1 ,
	};

	// recursive call on children
	OWNET_dirprocess(this_pass->h, filename, Count_element, &next_pass);
	
	this_pass->number += next_pass.number ;
}

long Tree_size(OWNET_HANDLE owh)
{
	struct pass_on first_pass ;

	first_pass.h = owh;
	first_pass.depth = 0;
	first_pass.number = 1 ;

	OWNET_dirprocess(owh, one_wire_path, Count_element, &first_pass);

	return first_pass.number;
}

int main(int argc, char **argv)
{
	long reps ;
	OWNET_HANDLE owh;
	parse_command_line(argc, argv);

	if ((owh = OWNET_init(owserver_address)) < 0) {
		printf("OWNET_init(%s) failed.\n", owserver_address);
		exit(1);
	}

	for ( reps = 0 ; reps < number_reps ; ++ reps ) {
		printf("%.8ld = %.8ld\n",reps, Tree_size(owh) ) ;
	}

	OWNET_close(owh);
	return 0;
}

