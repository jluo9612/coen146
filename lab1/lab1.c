#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

	FILE* fin = fopen(argv[1], "r");
	FILE* fout = fopen(argv[2], "w");
	if (fin == NULL || fout == NULL) {
		printf("Cannot open file!\n");
		exit(0);
	}

	char buff[10];

	while (!feof(fin)) {
		size_t numel = fread(buff, 1, 10, fin);
		fwrite(buff, 1, numel, fout);
	}

	fclose(fin);
	fclose(fout);

	return 0;
}
