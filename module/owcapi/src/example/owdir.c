#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <owcapi.h>

#if OW_USB
const char DEFAULT_ARGV[] = "-u";
#else
const char DEFAULT_ARGV[] = "-s 127.0.0.1:1234";
#endif

const char DEFAULT_PATH[] = "/";

int main(int argc, char **argv)
{
  ssize_t rc;
  ssize_t ret = 0;
  size_t len;
  char *buffer = NULL;
  const char *path = DEFAULT_PATH;

  if(argc < 2) {
    printf("%s started without any arguments.\n", argv[0]);
    printf("Starting with parameter \"%s\" as default\n", DEFAULT_ARGV);

    if((rc = OW_init(DEFAULT_ARGV)) < 0) {
      perror("OW_init failed.\n");
      ret = rc;
      goto cleanup;
    }
  } else {

    /* steal first argument and treat it as a path (if not beginning with '-') */
    if(argv[1][0] != '-') {
      path = argv[1];
      argv = &argv[1];
      argc--;
    }

    if((rc = OW_init_args(argc, argv)) < 0) {
      printf("OW_init_args failed with rd=%d\n", rc);
      ret = rc;
      goto cleanup;
    }
  }

  if((rc = OW_get(path, &buffer, &len)) < 0) {
    printf("OW_get failed with rc=%d\n", rc);
    ret = rc;
    goto cleanup;
  }

  if(!buffer) {
    printf("OW_get: buffer is empty\n");
    ret = 1;
    goto cleanup;
  }

  printf("OW_get() returned rc=%d, len=%d\n", (int)rc, (int)len);
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

