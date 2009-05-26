#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include <owcapi.h>

const char DEFAULT_ARGV[] = "--fake 28 --fake 10";

const char DEFAULT_PATH[] = "/";

/* Takes a path and filename and prints the 1-wire value */
/* makes sure the bridging "/" in the path is correct */
/* watches for total length and free allocated space */
void GetValue(const char *path, const char *name)
{
	char fullpath[PATH_MAX + 1];
	int length_left = PATH_MAX;

	char *get_buffer;
	ssize_t get_return;
	size_t get_length;

	/* Check arguments and lengths */
	if (path == NULL || strlen(path) == 0) {
		path = "/";
	}
	if (name == NULL) {
		name = "";
	}

	fullpath[PATH_MAX] = '\0';	// make sure final 0 byte

	strncpy(fullpath, path, length_left);
	length_left -= strlen(fullpath);

	if (length_left > 0) {
		if (fullpath[PATH_MAX - length_left - 1] != '/') {
			strcat(fullpath, "/");
			--length_left;
		}
	}
	if (name[0] == '/') {
		++name;
	}

	strncpy(&fullpath[PATH_MAX - length_left], name, length_left);

	get_return = OW_get(fullpath, &get_buffer, &get_length);
	if (get_return < 0) {
		printf("ERROR (%d) reading %s (%s)\n", errno, fullpath, strerror(errno));
	} else {
		printf("%s has value %s (%d bytes)\n", fullpath, get_buffer, (int) get_length);
		free(get_buffer);
	}
}

int main(int argc, char **argv)
{
	const char *path = DEFAULT_PATH;

	ssize_t dir_return;
	char *dir_buffer = NULL;
	char *dir_buffer_copy = NULL;
	size_t dir_length;
	char **d_buffer;
	char *dir_member;

	/* ------------- INITIALIZATIOB ------------------ */
	/* steal first argument and treat it as a path (if not beginning with '-') */
	if ((argc > 1) && (argv[1][0] != '-')) {
		path = argv[1];
		argv = &argv[1];
		argc--;
	}

	if (argc < 2) {
		printf("Starting with extra parameter \"%s\" as default\n", DEFAULT_ARGV);

		if (OW_init(DEFAULT_ARGV) < 0) {
			printf("OW_init error = %d (%s)\n", errno, strerror(errno));
			goto cleanup;
		}
	} else {
		if (OW_init_args(argc, argv) < 0) {
			printf("OW_init error = %d (%s)\n", errno, strerror(errno));
			goto cleanup;
		}
	}

	/* ------------- DIRECTORY PATH ------------------ */
	dir_return = OW_get(path, &dir_buffer, &dir_length);
	if (dir_return < 0) {
		printf("DIRECTORY ERROR (%d) reading %s (%s)\n", errno, path, strerror(errno));
		goto cleanup;
	} else {
		printf("Directory %s has members %s (%d bytes)\n", path, dir_buffer, (int) dir_length);
	}

	printf("\n");

	/* ------------- GO THROUGH DIR ------------------ */
	d_buffer = &dir_buffer;
	dir_buffer_copy = dir_buffer;
	while ((dir_member = strsep(d_buffer, ",")) != NULL) {
		switch (dir_member[0]) {
		case '1':
			if (dir_member[1] == '0') {
				// DS18S20 (family code 10)
				GetValue(dir_member, "temperature");
			}
			//fall through
		case '0':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			{
				GetValue(dir_member, "type");
			}
			break;
		default:
			break;
		}
	}
	
	/* ------------- STATIC PATHS   ------------------ */
	GetValue("system/process", "pid");
	GetValue("badPath", "badName");
	
  cleanup:
	/* ------------- DONE -- CLEANUP ----------------- */
	OW_finish();

	if (dir_buffer_copy) {
		free(dir_buffer_copy);
		dir_buffer_copy = NULL;
	}
	
	return 0;
}
