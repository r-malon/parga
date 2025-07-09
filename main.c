#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <iso646.h>

#define ERROR(msg) do { fputs(msg, stderr); exit(EXIT_FAILURE); } while (0)
#define CHECKMEM(addr, errmsg) do {		\
	if (addr < 0 or addr >= MEMSIZE)	\
		ERROR(errmsg);					\
} while (0)

typedef enum {
	TOKEN_OP,
	TOKEN_STATEMENT,
	TOKEN_STRING='A',
	TOKEN_NUMBER='0'
} TokenType;

typedef struct Token {
	TokenType type;
	struct Token *next;
	union {
		char *str;
		double num;
		void *(*op)();
	};
} Token;

typedef struct Stack {
	Token *top;
	size_t size;
} Stack;

typedef Token *(*parse)(FILE *);

Token *parse_number(FILE *);
Token *parse_string(FILE *);
Token *parse_op(FILE *);

parse jumptable[128] = {
	[' '] = parse_op,
	['0' ... '9'] = parse_number,
	['a' ... 'z'] = parse_string,
	['A' ... 'Z'] = parse_string,
	['"'] = parse_string,
	['\''] = parse_string,
	['\\'] = parse_string,
	['+'] = parse_op,
	['-'] = parse_op,
	['*'] = parse_op,
	['/'] = parse_op,
	['%'] = parse_op,
	['='] = parse_op,
	['<'] = parse_op,
	['>'] = parse_op,
	['!'] = parse_op,
	['&'] = parse_op,
	['|'] = parse_op,
	['^'] = parse_op,
	['~'] = parse_op
};

// dup, swap, over, rot

void
push(Stack *s, Token *t) {
	if (s->top != NULL)
		t->next = s->top;
	s->top = t;
}

Token *
pop(Stack *s) {
	Token *popped = s->top;
	s->top = s->top->next; // cleanup
	return popped;
}

int
main(int argc, char *argv[])
{
	int c;
	FILE *fp;
	Stack stack;

	if (argc > 1) {
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			fprintf(stderr, "Could not open file '%s'.\n", argv[1]);
			return EXIT_FAILURE;
		}
	} else {
		fp = stdin;
	}
// lexing
	while ((c = getc(fp)) != EOF) {
		push(&stack, jumptable[c](fp));
	}

	if (fp != stdin)
		fclose(fp);

	return 0;
}
