#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include <ctype.h>

#include <ownetapi.h>

//------------- Globals vaiables ----------
char *owserver_address = "4304";
char *one_wire_path = "/";

//------------- Usage information --------
void usage(int argc, char **argv)
{
	printf("%s shows examples of directory and read functions\n", argv[0]);
	printf("\n");
	printf("Usage of %s:\n", argv[0]);
	printf("\t%s -s owserver_address\n", argv[0]);
	printf("\t\towserver_address -- tcp/ip address:port of owserver\n");
	printf("\t\t\te.g. 192.168.0.77:3000 or just port number\n");
	printf("\t\t\tdefault localhost:4304\n");
	printf("\n");
	printf("see http://www.owfs.org for information on owserver.\n");
	exit(1);
}

//------------- Command line parsing -----
void parse_command_line(int argc, char **argv)
{
	int argc_index;
	int next_is_owserver = 1;
	if (argc < 1) {
		usage(argc, argv);
	}
	for (argc_index = 1; argc_index < argc; ++argc_index) {
		if (strcmp(argv[argc_index], "-h") == 0) {
			usage(argc, argv);
		} else if (strcmp(argv[argc_index], "--help") == 0) {
			usage(argc, argv);
		} else if (strcmp(argv[argc_index], "-s") == 0) {
			next_is_owserver = 1;
		} else if (strcmp(argv[argc_index], "--server") == 0) {
			next_is_owserver = 1;
		} else {
			if (next_is_owserver) {
				owserver_address = argv[argc_index];
				next_is_owserver = 0;
			} else {
				fprintf(stderr,"Extra argument <%s> in command line\n",argv[argc_index]);
				exit(1) ;			
			}
		}
	}
}

//------------- Example-specific ---------
struct pass_on {
	OWNET_HANDLE h;
	int depth;
};

// directory_element callback function
void Show_property(void *v, const char *filename)
{
	// cast the void pointer to a known structure pointer
	struct pass_on *this_pass = v;
	// set up a structure to pass to future call-back
	struct pass_on next_pass = {
		this_pass->h,
		this_pass->depth + 1,
	};
	char * read_data = NULL ;
	int read_length = OWNET_read(this_pass->h, filename, &read_data);

	// space in to level (for example)
	int indent_index;
	for (indent_index = 0; indent_index < this_pass->depth; ++indent_index) {
		printf(" ");
	}

	// print this filename
	printf("%s", filename);

	if ( read_length >= 0 ) {
		int i ;
		for ( i = 0 ; i < read_length ; ++i ) {
			if ( ! isprint( (int) read_data[i] ) ) {
				read_data[i] = '.' ;
			}
		}
		printf("\t<%s>",read_data);
	}
	if ( read_data != NULL ) {
		free( read_data ) ;
	}
	printf("\n") ;

	// recursive call on children
	OWNET_dirprocess(this_pass->h, filename, Show_property, &next_pass);
}

// directory_element callback function
void Show_device(void *v, const char *filename)
{
	// cast the void pointer to a known structure pointer
	struct pass_on *this_pass = v;
	// set up a structure to pass to future call-back
	struct pass_on next_pass = {
		this_pass->h,
		this_pass->depth + 1,
	};

	// space in to level (for example)
	int indent_index;
	for (indent_index = 0; indent_index < this_pass->depth; ++indent_index) {
		printf(" ");
	}

	// print this filename
	printf("%s\n", filename);

	// Only continue for real devices
	if ( !isxdigit(filename[1]) || !isxdigit(filename[2]) ) {
		return ;
	}

	// recursive call on children
	OWNET_dirprocess(this_pass->h, filename, Show_property, &next_pass);
}

int main(int argc, char **argv)
{
	ssize_t ret = 0;
	OWNET_HANDLE owh;
	struct pass_on first_pass;

	parse_command_line(argc, argv);

	if ((owh = OWNET_init(owserver_address)) < 0) {
		printf("OWNET_init(%s) failed.\n", owserver_address);
		exit(1);
	}

	first_pass.h = owh;
	first_pass.depth = 0;

	ret = OWNET_dirprocess(owh, one_wire_path, Show_device, &first_pass);
	if (ret < 0) {
		printf("OWNET_dirprocess error: %ld)\n", ret);
	}

	OWNET_close(owh);
	return 0;
}
