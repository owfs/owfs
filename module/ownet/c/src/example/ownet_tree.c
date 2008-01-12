#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <ownetapi.h>

const char DEFAULT_ARGV[] = "4304";

const char DEFAULT_PATH[] = "/";

struct pass_on {
    OWNET_HANDLE h ;
    int depth ;
} ;

void Show_element( void * v, const char * filename )
{
    struct pass_on * this_pass = v ;
    struct pass_on next_pass = { 
        this_pass->h,
        this_pass->depth+1 ,
    } ;

    int index ;
    
    for ( index=0 ; index < this_pass->depth ; ++index ) {
        printf(" ");
    }
    printf("%s\n",filename ) ;
    OWNET_dirprocess( this_pass->h, filename, Show_element, &next_pass) ;
}

int main(int argc, char **argv)
{
  ssize_t rc;
  ssize_t ret = 0;
  size_t len;
  char *buffer = NULL;
  const char *path = DEFAULT_PATH;
  OWNET_HANDLE owh;
  struct pass_on first_pass ;

  printf("Usage example:\n");
  printf("%s\n", argv[0]);
  printf("%s /10.67C6697351FF\n", argv[0]);
  printf("%s / 6666\n", argv[0]);
  printf("Default parameters are:  %s %s %s\n", argv[0], DEFAULT_PATH, DEFAULT_ARGV);

  /* steal first argument and treat it as a path (if not beginning with '-') */
  if((argc > 1) && (argv[1][0] != '-')) {
    path = argv[1];
    argv = &argv[1];
    argc--;
  }

  if(argc < 2) {
    printf("Starting with extra parameter \"%s\" as default\n", DEFAULT_ARGV);
    if((owh = OWNET_init(DEFAULT_ARGV)) < 0) {
      perror("OWNET_init failed.\n");
      goto cleanup;
    }
  } else {
    if((owh = OWNET_init(argv[1])) < 0) {
      printf("OWNET_init(%s) failed.\n", argv[1]);
      exit(1) ;
    }
  }

  first_pass.h = owh ;
  first_pass.depth = 0 ;

  len = OWNET_dirprocess(owh, path, Show_element, &first_pass);
  if(len<0) {
    printf("OWNET_dirprocess error: %d)\n", len);
  }

cleanup:
  OWNET_close(owh);
  return 0 ;
}

