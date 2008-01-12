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
    printf("%s shows examples of directory and read functions\n",basename(argv[0]));
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
int main(int argc, char **argv)
{
  ssize_t rc;
  ssize_t ret = 0;
  size_t len;
  char *buffer = NULL;
  OWNET_HANDLE owh;

    parse_command_line(argc,argv) ;

    if((owh = OWNET_init(owserver_address)) < 0) {
        printf("OWNET_init(%s) failed.\n", owserver_address);
        exit(1) ;
    }


  len = OWNET_dirlist(owh, one_wire_path, &buffer);
  if((len<0) || !buffer) {
    printf("OWNET_dirlist: buffer is empty (buffer=%p len=%d)\n", buffer, len);
    goto cleanup;
  }

  printf("OWNET_dirlist() returned len=%d\n", (int)len);
  buffer[len] = 0;
  printf("------- buffer content -------\n");
  printf("%s\n", buffer);
  printf("------------------------------\n");
  if(buffer) {
    free(buffer);
    buffer = NULL;
  }

  len = OWNET_read(owh, "10.67C6697351FF/type", &buffer);
  if((len<0) || !buffer) {
    printf("OWNET_read: buffer is empty (buffer=%p len=%d)\n", buffer, len);
    goto cleanup;
  }
  printf("OWNET_read() returned len=%d\n", (int)len);
  buffer[len] = 0;
  printf("------- buffer content (10.67C6697351FF/type) -------\n");
  printf("%s\n", buffer);
  printf("------------------------------\n");
  if(buffer) {
    free(buffer);
    buffer = NULL;
  }

 cleanup:
  OWNET_close(owh);
  
  if(buffer) {
    free(buffer);
    buffer = NULL;
  }

  exit(ret);
}
