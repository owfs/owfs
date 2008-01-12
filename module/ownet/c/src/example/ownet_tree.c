#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <ownetapi.h>

//------------- Global vaiables ----------
char * owserver_address = "4304";
char * one_wire_path = "/";

//------------- Usage information --------
void usage( int argc, char ** argv )
{
    printf("%s is an example of the OWNET_process function\n",basename(argv[0]));
    printf("\tevery directory path is traversed recursively\n");
    printf("\n");
    printf("Usage of %s:\n",basename(argv[0]));
    printf("\t%s -s owserver_address one_wire_path\n",argv[0]);
    printf("\t\towserver_address -- tcp/ip address:port of owserver\n");
    printf("\t\t\te.g. 192.168.0.77:3000 or just port number\n");
    printf("\t\t\tdefault localhost:4304\n");
    printf("\t\tone_wire_path    -- OWFS-style 1-wire address.\n");
    printf("\t\t\te.g.: / for root 1-wire bus listing (default)\n");
    printf("\t\t\tor /10.67C6697351FF for a device directory\n");
    printf("\n");
    printf("see http://www.owfs.org for information on owserver.\n");
    exit(1) ;
}

//------------- Command line parsing -----
void parse_command_line( int argc, char ** argv )
{
    int argc_index ;
    int next_is_owserver = 0 ;
    if ( argc == 1 ) {
        usage(argc,argv);
    }
    for (argc_index = 1 ; argc_index < argc ; ++argc_index ) {
        if ( strcmp(argv[argc_index],"-h")==0) {
            usage(argc,argv) ;
        } else if ( strcmp(argv[argc_index],"--help")==0) {
            usage(argc,argv) ;
        } else if ( strcmp(argv[argc_index],"-s")==0) {
             next_is_owserver = 1 ;
        } else if ( strcmp(argv[argc_index],"--server")==0) {
             next_is_owserver = 1 ;
        } else {
            if ( next_is_owserver ) {
                owserver_address = argv[argc_index] ;
                next_is_owserver = 0 ;
            } else {
                one_wire_path = argv[argc_index] ;
            }
        }
    }
}

//------------- Example-specific ---------
struct pass_on {
    OWNET_HANDLE h ;
    int depth ;
} ;

// directory_element callback function
void Show_element( void * v, const char * filename )
{
    // cast the void pointer to a known structure pointer
    struct pass_on * this_pass = v ;
    // set up a structure to pass to future call-back
    struct pass_on next_pass = { 
        this_pass->h,
        this_pass->depth+1 ,
    } ;

    // space in to level (for example)
    int index ;
    for ( index=0 ; index < this_pass->depth ; ++index ) {
        printf(" ");
    }

    // print this filename
    printf("%s\n",filename ) ;

    // recursive call on children
    OWNET_dirprocess( this_pass->h, filename, Show_element, &next_pass) ;
}

int main(int argc, char **argv)
{
    ssize_t ret = 0;
    OWNET_HANDLE owh;
    struct pass_on first_pass ;
    
    parse_command_line(argc,argv) ;

    if((owh = OWNET_init(owserver_address)) < 0) {
        printf("OWNET_init(%s) failed.\n", owserver_address);
        exit(1) ;
    }

    first_pass.h = owh ;
    first_pass.depth = 0 ;
    
    ret = OWNET_dirprocess(owh, one_wire_path, Show_element, &first_pass);
    if(ret<0) {
        printf("OWNET_dirprocess error: %d)\n", ret);
    }
    
    OWNET_close(owh);
    return 0 ;
}

