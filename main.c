#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <iso646.h>

#define COMMENT_CHAR ';'
#define ERROR(msg) do { fputs(msg, stderr); exit(EXIT_FAILURE); } while (0)

typedef enum {
	TOKEN_OP,
	TOKEN_STATEMENT,
	TOKEN_STRING='A',
	TOKEN_NUMBER='0'
} TokenType;

typedef struct Function {
	int arity;
	void *(*op)();
} Function;

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

Token *parse_number(FILE *fp) {
	int c;
	Token *t = malloc(sizeof (Token));
	t->type = TOKEN_NUMBER;
	while ((c = getc(fp)) != ' ') {
		t->num *= 10;
		t->num += c - '0';
		printf("%f\n", t->num);
	}
	return t;
}

Token *parse_string(FILE *fp) {
	int i;
	while (getc(fp) != '"') i++;
	fseek(fp, 0, SEEK_SET);
	Token *t = malloc(sizeof (Token));
	t->type = TOKEN_STRING;
	t->str = malloc(i);
	memccpy(t->str, fp, '"', 1024);
	printf("%s\n", t->str);
	return t;
}

Token *parse_op(FILE *fp) {
	int c;
	Token *t = malloc(sizeof (Token));
	t->type = TOKEN_OP;
	while ((c = getc(fp)) != ' ') c += c;
	return t;
}

parse jumptable[128] = {
	[' '] = parse_op,
	['0' ... '9'] = parse_number,
	['a' ... 'z'] = parse_op,
	['A' ... 'Z'] = parse_op,
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
	['~'] = parse_op,
//	[COMMENT_CHAR] = parse_comment
};

// dup, swap, over, rot, tuck, pick, roll

void
push(Stack *s, Token *t) {
	if (s->top != NULL)
		t->next = s->top;
	s->top = t;
	s->size++;
}

Token *
pop(Stack *s) {
	if (s->top == NULL)
		return NULL;
	Token *popped = s->top;
	s->top = s->top->next; // cleanup
	s->size--;
	return popped;
}
/*
inline unsigned int hash(unsigned int K) { 
	return (a * K) >> (w - m);
}
*/
void token_free(Token *t) {
	if (t->type == TOKEN_STRING && t->str != NULL)
		free(t->str);
	free(t);
}

void stack_free(Stack *s) {
	while (s->top != NULL)
		token_free(pop(s));
}

/* SRC -> DEST
void stack_read(FILE *fp, Stack *s);
void stack_write(Stack *s, FILE *fp);
*/

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
		printf("%u\n", c);
		printf("XX %u", jumptable[c](fp));
		push(&stack, jumptable[c](fp));
		if (stack.top->type == TOKEN_STRING)
			printf("%s\n", stack.top->str);
		if (stack.top->type == TOKEN_NUMBER)
			printf("%f\n", stack.top->num);
	}

	if (fp != stdin)
		fclose(fp);

	stack_free(&stack);

	for (int i = 0; i < stack.size; i++) {
		printf("OUT: %s", pop(&stack)->str);
	}
	return 0;
}
