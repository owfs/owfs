#include <stdio.h>
#include <stdlib.h>

#include <owcapi.h>

const char DEFAULT_ARGV[] = "-u";

int main(int argc, char **argv)
{
  int rc;
  int ret = 0;
  size_t len;
  char *buffer = NULL;

  if(argc < 2) {
    printf("%s started without any arguments.\n", argv[0]);
    printf("Starting with parameter \"%s\" as default\n", DEFAULT_ARGV);

    if((rc = OW_init(DEFAULT_ARGV)) < 0) {
      printf("OW_init failed.\n");
      ret = rc;
      goto cleanup;
    }
  } else {
    if((rc = OW_init_args(argc, argv)) < 0) {
      printf("OW_init_args failed.\n");
      ret = rc;
      goto cleanup;
    }
  }

  if((rc = OW_get( "/", &buffer, &len)) < 0) {
    printf("OW_get failed.\n");
    ret = rc;
    goto cleanup;
  }

  if(!buffer) {
    printf("OW_get return buffer==NULL ?\n");
    ret = 1;
    goto cleanup;
  }

  printf("OW_get() returned rc=%d, len=%d\n", rc, len);
  buffer[len] = 0;
  printf("------- buffer content -------\n");
  printf("%s\n", buffer);
  printf("------------------------------\n");
  
 cleanup:
  OW_finish();

  if(buffer) {
    free(buffer);
    buffer = NULL;
  }
  exit(ret);
}

