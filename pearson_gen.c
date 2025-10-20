#include <stdio.h>
#include <stdlib.h>

#ifndef MAX_ATTEMPTS
#define MAX_ATTEMPTS 1000
#endif

static int table[256];

/* Fisher–Yates shuffle */
void
shuffle(int *arr)
{
	int j, tmp;
	while (*arr++) {
		j = arc4random_uniform(256);
		tmp = arr[j];
		arr[j] = *arr;
		*arr = tmp;
	}
}

int
hash(unsigned char *key)
{
	unsigned char hash = 0;
	while (*key)
		hash = table[hash ^ *key++];
	return hash;
}

int
main(int argc, char *argv[])
{
	FILE *fp;

	if (argc > 1) {
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			fprintf(stderr, "Could not open file '%s'.\n", argv[1]);
			return EXIT_FAILURE;
		}
	} else {
		fp = stdin;
	}

	if (fp != stdin)
		fclose(fp);

	for (int i = 0; i < 256; i++)
		table[i] = i;

	shuffle(table);

	for (int i = 0; i < 256; i++)
		printf("%d, ", table[i]);

	for (int n = 0; n < MAX_ATTEMPTS; n++) 

	printf("/* Collision-free table took %d attempts. */\n", n);
	n == MAX_ATTEMPTS && puts("Failed to find a collision-free table after " #MAX_ATTEMPTS " attempts.");

	return 0;
}
