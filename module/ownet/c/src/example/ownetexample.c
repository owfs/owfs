#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <ownetapi.h>

const char DEFAULT_ARGV[] = "4304";

const char DEFAULT_PATH[] = "/";

int main(int argc, char **argv)
{
  ssize_t rc;
  ssize_t ret = 0;
  size_t len;
  char *buffer = NULL;
  const char *path = DEFAULT_PATH;
  OWNET_HANDLE owh;

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
      goto cleanup;
    }
  }

  len = OWNET_dirlist(owh, path, &buffer);
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

