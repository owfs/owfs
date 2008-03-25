#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <ownetapi.h>

//------------- Global vaiables ----------
char *owserver_address = "4304";
char *one_wire_path = "/";

//------------- Usage information --------
void usage(int argc, char **argv)
{
    printf("%s shows examples of directory and read functions\n", argv[0]);
	printf("\n");
    printf("Usage of %s:\n", argv[0]);
    printf("\t%s -s owserver_address one_wire_path\n", argv[0]);
	printf("\t\towserver_address -- tcp/ip address:port of owserver\n");
	printf("\t\t\te.g. 192.168.0.77:3000 or just port number\n");
	printf("\t\t\tdefault localhost:4304\n");
	printf("\t\tone_wire_path    -- OWFS-style 1-wire address.\n");
	printf("\t\t\te.g.: / for root 1-wire bus listing (default)\n");
	printf("\t\t\tor /10.67C6697351FF for a device directory\n");
	printf("\n");
	printf("see http://www.owfs.org for information on owserver.\n");
	exit(1);
}

//------------- Command line parsing -----
void parse_command_line(int argc, char **argv)
{
	int argc_index;
	int next_is_owserver = 0;
	if (argc < 2) {
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
				one_wire_path = argv[argc_index];
			}
		}
	}
}

//------------- Example-specific ---------
int main(int argc, char **argv)
{
	ssize_t rc;
	ssize_t ret = 0;
	size_t len;
    char *dir_buffer = NULL;
    char ** d_buffer ;
    char * dir_member ;
    OWNET_HANDLE owh;

    parse_command_line(argc, argv);

	if ((owh = OWNET_init(owserver_address)) < 0) {
		printf("OWNET_init(%s) failed.\n", owserver_address);
		exit(1);
	}

	len = OWNET_dirlist(owh, one_wire_path, &dir_buffer);
	if ((len < 0) || dir_buffer==NULL) {
		printf("OWNET_dirlist: buffer is empty (buffer=%p len=%d)\n", dir_buffer, len);
		goto cleanup;
	}

	printf("OWNET_dirlist() returned len=%d\n", (int) len);
    dir_buffer[len] = 0; // make sure last null char
	printf("------- buffer content -------\n");
	printf("%s\n", dir_buffer);
	printf("------------------------------\n");

    d_buffer = &dir_buffer ;
    while ( (dir_member=strsep(d_buffer,",")) ) {
        switch ( dir_member[1] ) {
            case '1':
                if ( dir_member[2] == '0' ) {
                    char tpath[128] ;
                    char *type_buffer = NULL;

                    strcpy( tpath, dir_member ) ;
                    strcat( tpath, "/temperature" ) ;
                    len = OWNET_read(owh, tpath, &type_buffer);
                    if ((len < 0) || type_buffer==NULL) {
                        printf("OWNET_read: buffer is empty (buffer=%p len=%d)\n", type_buffer, len);
                        goto cleanup;
                    }                    printf("OWNET_read() returned len=%d\n", (int) len);
                    type_buffer[len] = 0;
                    printf("------- buffer content (%s) -------\n",tpath);
                    printf("%s\n", type_buffer);
                    printf("------------------------------\n");
                    if (type_buffer) {
                        free(type_buffer);
                    }
                }
                //fall through
            case '0': case '2': case '3':
            case '4': case '5': case '6': case '7':
            case '8': case '9': case 'A': case 'B':
            case 'C': case 'D': case 'E': case 'F':
            {
                char tpath[128] ;
                char *type_buffer = NULL;

                strcpy( tpath, dir_member ) ;
                strcat( tpath, "/type" ) ;
                len = OWNET_read(owh, tpath, &type_buffer);
                if ((len < 0) || type_buffer==NULL) {
                    printf("OWNET_read: buffer is empty (buffer=%p len=%d)\n", type_buffer, len);
                    goto cleanup;
                }                printf("OWNET_read() returned len=%d\n", (int) len);
                type_buffer[len] = 0;
                printf("------- buffer content (%s) -------\n",tpath);
                printf("%s\n", type_buffer);
                printf("------------------------------\n");
                if (type_buffer) {
                    free(type_buffer);
                }
            }
            break ;
            default:
                break ;
        }
    }

  cleanup:
	OWNET_close(owh);

	if (dir_buffer) {
		free(dir_buffer);
		dir_buffer = NULL;
	}

	exit(ret);
}
