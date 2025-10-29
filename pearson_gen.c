#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_ATTEMPTS 1000
#define MAX_KEYWORDS 256
#define MAX_LINE 64

static uint8_t table[256];
static char keywords[MAX_KEYWORDS][MAX_LINE];
static int n_keywords;

/* Fisher–Yates shuffle */
static void
shuffle(void)
{
	int i, j;
	uint8_t tmp;

	for (i = 255; i > 0; i--) {
		j = arc4random_uniform(i + 1);
		tmp = table[j];
		table[j] = table[i];
		table[i] = tmp;
	}
}

static uint8_t
hash(const char *s)
{
	uint8_t h = 0;

	while (*s)
		h = table[h ^ (uint8_t)*s++];
	return h;
}

static bool
has_collision(void)
{
	bool seen[256] = {false};
	uint8_t h;
	int i;

	for (i = 0; i < n_keywords; i++) {
		h = hash(keywords[i]);
		if (seen[h])
			return true;
		seen[h] = true;
	}
	return false;
}

int
main(void)
{
	int i, n;

	while (fgets(keywords[n_keywords], MAX_LINE, stdin)) {
		keywords[n_keywords][strcspn(keywords[n_keywords], "\n")] = 0;
		if (keywords[n_keywords][0])
			n_keywords++;
	}

	for (i = 0; i < 256; i++) table[i] = i;

	for (n = 1; n <= MAX_ATTEMPTS; n++) {
		shuffle();
		if (!has_collision()) break;
	}

	if (n > MAX_ATTEMPTS) {
		fprintf(stderr, "Failed after %d attempts.\n", MAX_ATTEMPTS);
		return 1;
	}

	printf("/* Collision-free table took %d attempts. */\n", n);
	puts("static const uint8_t table[] = {");
	for (i = 0; i < 256; i++)
		printf("%3u,%c", table[i], (i % 16 == 15) ? '\n' : ' ');
	puts("};");

	return 0;
}
