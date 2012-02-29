#include "libsass.h"
#include <stdio.h>

int main(int argc, char **argv)
{
	char *filename = argv[1];
	char *output = sass_file_compile(filename, 0);
	printf("%s", output);
	return 0;
}