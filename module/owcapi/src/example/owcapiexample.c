#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <owcapi.h>

const char DEFAULT_ARGV[] = "--fake 28 --fake 10";

const char DEFAULT_PATH[] = "/";

int main(int argc, char **argv)
{
  ssize_t rc;
  ssize_t ret = 0;
  size_t len;
  char *buffer = NULL;
  const char *path = DEFAULT_PATH;

  /* steal first argument and treat it as a path (if not beginning with '-') */
  if((argc > 1) && (argv[1][0] != '-')) {
    path = argv[1];
    argv = &argv[1];
    argc--;
  }

  if(argc < 2) {
    printf("Starting with extra parameter \"%s\" as default\n", DEFAULT_ARGV);

    if((rc = OW_init(DEFAULT_ARGV)) < 0) {
      perror("OW_init failed.\n");
      ret = rc;
      goto cleanup;
    }
  } else {

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

  if(!buffer || (len <= 0)) {
    printf("OW_get: buffer is empty (buffer=%p len=%d)\n", buffer, len);
    ret = 1;
    goto cleanup;
  }

  printf("OW_get() returned rc=%d, len=%d\n", (int)rc, (int)len);
  buffer[len] = 0;
  printf("------- buffer content -------\n");
  printf("%s\n", buffer);
  printf("------------------------------\n");
  if(buffer) {
    free(buffer);
    buffer = NULL;
  }

  if((rc = OW_get("10.67C6697351FF/type",&buffer,&len)) < 0) {
    printf("OW_get(/type) failed with rc=%d\n", rc);
    ret = rc;
    goto cleanup;
  }
  if(!buffer || (len <= 0)) {
    printf("OW_get: buffer is empty (buffer=%p len=%d)\n", buffer, len);
    ret = 1;
    goto cleanup;
  }
  printf("OW_get() returned rc=%d, len=%d\n", (int)rc, (int)len);
  buffer[len] = 0;
  printf("------- buffer content (10.67C6697351FF/type) -------\n");
  printf("%s\n", buffer);
  printf("------------------------------\n");
  if(buffer) {
    free(buffer);
    buffer = NULL;
  }

  if((rc = OW_get("10.67C6697351FF/temperature",&buffer,&len)) < 0) {
    printf("OW_get(/temperature) failed with rc=%d\n", rc);
    ret = rc;
    goto cleanup;
  }
  if(!buffer || (len <= 0)) {
    printf("OW_get: buffer is empty (buffer=%p len=%d)\n", buffer, len);
    ret = 1;
    goto cleanup;
  }
  printf("OW_get() returned rc=%d, len=%d\n", (int)rc, (int)len);
  buffer[len] = 0;
  printf("------- buffer content (10.67C6697351FF/temperature) -------\n");
  printf("%s\n", buffer);
  printf("------------------------------\n");
  if(buffer) {
    free(buffer);
    buffer = NULL;
  }

  if((rc = OW_get("system/adapter/name.ALL",&buffer,&len)) < 0) {
    printf("OW_get(system/adapter/name.ALL) failed with rc=%d\n", rc);
    ret = rc;
    goto cleanup;
  }
  if(!buffer || (len <= 0)) {
    printf("OW_get: buffer is empty (buffer=%p len=%d)\n", buffer, len);
    ret = 1;
    goto cleanup;
  }
  printf("OW_get() returned rc=%d, len=%d\n", (int)rc, (int)len);
  buffer[len] = 0;
  printf("------- buffer content (system/adapter/name.ALL) -------\n");
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

