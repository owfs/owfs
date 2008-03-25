#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <owcapi.h>

const char DEFAULT_ARGV[] = "--fake 28 --fake 10";

const char DEFAULT_PATH[] = "/";

int main(int argc, char **argv)
{
	ssize_t rc;
	ssize_t ret = 0;
	size_t len;
    char *dir_buffer = NULL;
    char ** d_buffer ;
    char * dir_member ;
    char * buffer = NULL ;
    const char *path = DEFAULT_PATH;

	/* steal first argument and treat it as a path (if not beginning with '-') */
	if ((argc > 1) && (argv[1][0] != '-')) {
		path = argv[1];
		argv = &argv[1];
		argc--;
	}

	if (argc < 2) {
		printf("Starting with extra parameter \"%s\" as default\n", DEFAULT_ARGV);

		if ((rc = OW_init(DEFAULT_ARGV)) < 0) {
			perror("OW_init failed.\n");
			ret = rc;
			goto cleanup;
		}
	} else {

		if ((rc = OW_init_args(argc, argv)) < 0) {
			printf("OW_init_args failed with rd=%d\n", rc);
			ret = rc;
			goto cleanup;
		}
	}

	if ((rc = OW_get(path, &dir_buffer, &len)) < 0) {
		printf("OW_get failed with rc=%d\n", rc);
		ret = rc;
		goto cleanup;
	}

	if (dir_buffer==NULL || (len <= 0)) {
		printf("OW_get: buffer is empty (buffer=%p len=%d)\n", dir_buffer, len);
		ret = 1;
		goto cleanup;
	}

	printf("OW_get() returned rc=%d, len=%d\n", (int) rc, (int) len);
	dir_buffer[len] = 0;
	printf("------- buffer content -------\n");
	printf("%s\n", dir_buffer);
	printf("------------------------------\n");

    d_buffer = &dir_buffer ;
    while ( (dir_member=strsep(d_buffer,","))!=NULL ) {
        switch ( dir_member[0] ) {
            case '1':
                if ( dir_member[1] == '0' ) {
                    char tpath[128] ;
                    char *type_buffer = NULL;

                    strcpy( tpath, dir_member ) ;
                    strcat( tpath, "temperature" ) ;
                    if ( OW_get(tpath, &type_buffer, &len) < 0 ) {
                        printf("OW_get: error\n" ) ;
                        break ;
                    }
                    printf("OW_get() returned len=%d\n", (int) len);
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
                strcat( tpath, "type" ) ;
                if ( OW_get( tpath, &type_buffer, &len ) < 0 ) {
                    printf("OW_get: error\n", type_buffer, len);
                    break ;
                }
                printf("OW_get() returned len=%d\n", (int) len);
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


	if ((rc = OW_get("system/adapter/name.ALL", &buffer, &len)) < 0) {
		printf("OW_get(system/adapter/name.ALL) failed with rc=%d\n", rc);
		ret = rc;
		goto cleanup;
	}
	if (!buffer || (len <= 0)) {
		printf("OW_get: buffer is empty (buffer=%p len=%d)\n", buffer, len);
		ret = 1;
		goto cleanup;
	}
	printf("OW_get() returned rc=%d, len=%d\n", (int) rc, (int) len);
	buffer[len] = 0;
	printf("------- buffer content (system/adapter/name.ALL) -------\n");
	printf("%s\n", buffer);
	printf("------------------------------\n");

  cleanup:
	OW_finish();

  if (buffer) {
      free(buffer);
      buffer = NULL;
  }
  if (dir_buffer) {
      free(dir_buffer);
      dir_buffer = NULL;
  }
  exit(ret);
}
