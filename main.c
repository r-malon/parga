#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FUNCTION_START '{'
#define FUNCTION_END '}'
#define STR_DELIM '\''
#define STACKSTR_DELIM '"'
#define COMMENT_DELIM '\\'
#define ERROR(msg) do { fputs(msg, stderr); exit(EXIT_FAILURE); } while (0)

#ifndef MAX_STRING_LEN
#define MAX_STRING_LEN 1024
#endif

#ifndef MAX_NUMBER_LEN
#define MAX_NUMBER_LEN 64
#endif

enum {
	TOKEN_STRING = 'A',
	TOKEN_NUMBER = '0',
	TOKEN_FUNCTION = 'f',
	TOKEN_STACK = 's',
	TOKEN_CHAR = 'c',
};

typedef struct Token Token;
typedef struct Stack Stack;
typedef Token *(*ParseFunc)(FILE *);

struct Stack {
	Token *top;
	size_t size;
};

struct Token {
	int type;
	Token *next;
	union {
		char c;
		char *str;
		double num;
		Stack *stack;
	};
};

static int parse_until(FILE *, Stack *, int (*)(int));
static Token *tokenize_char(char);
static Token *parse_number(FILE *);
static Token *parse_comment(FILE *);
static Token *parse_string(FILE *);
static Token *parse_stackstring(FILE *);
static Token *parse_function(FILE *);
static int char_to_digit(int, int);
static void push(Stack *, Token *);
static Token *pop(Stack *);
static void token_free(Token *);
static void stack_free(Stack *);

static ParseFunc jumptable[128] = {
	['0' ... '9'] = parse_number,
	['a' ... 'z'] = parse_function,
	['A' ... 'Z'] = parse_function,
	[STACKSTR_DELIM] = parse_stackstring,
	[STR_DELIM] = parse_string,
	[COMMENT_DELIM] = parse_comment,
	[FUNCTION_START] = parse_function,
	['+'] = parse_function,
	['-'] = parse_function,
	['*'] = parse_function,
	['/'] = parse_function,
	['%'] = parse_function,
	['='] = parse_function,
	['<'] = parse_function,
	['>'] = parse_function,
	['!'] = parse_function,
	['&'] = parse_function,
	['|'] = parse_function,
	['^'] = parse_function,
	['~'] = parse_function,
};

static int
char_to_digit(int c, int base)
{
	int digit;

	if (c >= '0' && c <= '9') {
		digit = c - '0';
	} else if (c >= 'A' && c <= 'Z') {
		digit = c - 'A' + 10;
	} else if (c >= 'a' && c <= 'z') {
		digit = c - 'a' + 10;
	} else {
		return -1;
	}
	return (digit < base) ? digit : -1;
}

static Token *
tokenize_char(char c)
{
	Token *t;

	t = malloc(sizeof(Token));
	if (!t)
		ERROR("Memory allocation failed\n");

	t->type = TOKEN_CHAR;
	t->next = NULL;
	t->c = c;
	return t;
}

static int
stackstring_end(int c)
{
	return c == STACKSTR_DELIM;
}

static int
function_end(int c)
{
	return c == FUNCTION_END;
}

static int
symmetric_end(int c)
{
	return c == STACKSTR_DELIM || c == STR_DELIM || c == COMMENT_DELIM;
}

static int
parse_until(FILE *fp, Stack *s, int (*stop_cond)(int))
{
	int c;
	Token *t;

	while ((c = fgetc(fp)) != EOF) {
		if (stop_cond && stop_cond(c)) {
			ungetc(c, fp);
			return c;
		}
		if (stop_cond == stackstring_end) {
			push(s, tokenize_char(c));
			continue;
		}
		if (isspace(c))
			continue;

		if ((c >= 0 && c < 128 && jumptable[c] != NULL)) {
			if (!symmetric_end(c))
				ungetc(c, fp);

			if ((t = jumptable[c](fp))) {
				push(s, t);

			#if DEBUG
				if (t->type == TOKEN_STRING) {
					fprintf(stderr, "Parsed string: '%s'\n", t->str);
				} else if (t->type == TOKEN_NUMBER) {
					fprintf(stderr, "Parsed number: %g\n", t->num);
				} else if (t->type == TOKEN_FUNCTION) {
					fprintf(stderr, "Parsed function\n");
				}
			#endif
			}
		}
		#if DEBUG
		else {
			fprintf(stderr, "Unrecognized character: %c (%d)\n", c, c);
		}
		#endif
	}
	return EOF;
}

static Token *
parse_number(FILE *fp)
{
	int c, buf_pos, base, digit;
	bool in_fractional;
	char buffer[MAX_NUMBER_LEN], *hash_pos, *start_pos, *p;
	double result, fractional_place;
	Token *t;

	buf_pos = 0;
	base = 10;
	result = 0.0;
	fractional_place = 1.0;
	in_fractional = false;

	t = malloc(sizeof(Token));
	if (!t)
		ERROR("Memory allocation failed\n");

	t->type = TOKEN_NUMBER;
	t->next = NULL;
	t->num = 0.0;

	/* Read all characters that could be part of a number */
	while ((c = fgetc(fp)) != EOF && buf_pos < MAX_NUMBER_LEN) {
		if (isdigit(c) || isalpha(c) || c == '.' || c == '#') {
			buffer[buf_pos++] = c;
		} else {
			ungetc(c, fp);
			break;
		}
	}
	buffer[buf_pos] = '\0';

	if (buf_pos == 0) {
		free(t);
		return NULL;
	}

	/* Base specification (Erlang-style: base#number) */
	hash_pos = strchr(buffer, '#');
	start_pos = buffer;

	if (hash_pos != NULL) {
		*hash_pos = '\0'; /* Temporarily terminate base part */
		base = 0;

		for (p = buffer; *p; p++) {
			if (!isdigit(*p))
				ERROR("Invalid base specification\n");
			base = base * 10 + (*p - '0');
		}

		if (base < 2 || base > 36)
			ERROR("Base must be between 2 and 36\n");

		start_pos = hash_pos + 1; /* Start parsing after the '#' */
	}

	for (p = start_pos; *p; p++) {
		if (*p == '.') {
			if (in_fractional)
				ERROR("Multiple decimal points in number\n");
			in_fractional = true;
			continue;
		}

		digit = char_to_digit(*p, base);
		if (digit == -1)
			ERROR("Invalid digit for specified base\n");

		if (in_fractional) {
			fractional_place /= base;
			result += digit * fractional_place;
		} else {
			result = result * base + digit;
		}
	}

	t->num = result;
	return t;
}

static Token *
parse_comment(FILE *fp) {
	int c;

	while ((c = fgetc(fp)) != EOF && c != COMMENT_DELIM);

	return NULL;
}

static Token *
parse_string(FILE *fp)
{
	int c, i;
	char buffer[MAX_STRING_LEN];
	Token *t;

	i = 0;

	while ((c = fgetc(fp)) != EOF && c != STR_DELIM && i < MAX_STRING_LEN - 1)
		buffer[i++] = c;

	if (c != STR_DELIM)
		ERROR("Unterminated string literal\n");

	buffer[i] = '\0';

	t = malloc(sizeof(Token));
	if (!t)
		ERROR("Memory allocation failed\n");

	t->type = TOKEN_STRING;
	t->next = NULL;
	t->str = malloc(i + 1);
	if (!t->str) {
		free(t);
		ERROR("Memory allocation failed\n");
	}

	strcpy(t->str, buffer);
	return t;
}

static Token *
parse_stackstring(FILE *fp)
{
	Token *t;
	Stack *s;

	t = malloc(sizeof(Token));
	if (!t)
		ERROR("Memory allocation failed\n");

	t->type = TOKEN_STACK;
	t->next = NULL;

	s = malloc(sizeof(Stack));
	if (!s) {
		free(t);
		ERROR("Memory allocation failed\n");
	}
	s->top = NULL;
	s->size = 0;

	if (parse_until(fp, s, stackstring_end) != STACKSTR_DELIM)
		ERROR("Unterminated string literal\n");

	/* Consume the closing quote */
	fgetc(fp);

	t->stack = s;
	return t;
}

static Token *
parse_function(FILE *fp)
{
	Token *t;
	Stack *s;

	t = malloc(sizeof(Token));
	if (!t)
		ERROR("Memory allocation failed\n");

	t->type = TOKEN_FUNCTION;
	t->next = NULL;

	s = malloc(sizeof(Stack));
	if (!s) {
		free(t);
		ERROR("Memory allocation failed\n");
	}
	s->top = NULL;
	s->size = 0;

	if (parse_until(fp, s, function_end) != FUNCTION_END)
		ERROR("Unterminated function definition\n");

	/* Consume the closing brace */
	fgetc(fp);

	t->stack = s;
	return t;
}

static void
push(Stack *s, Token *t)
{
	if (s->top != NULL)
		t->next = s->top;
	s->top = t;
	s->size++;
}

static Token *
pop(Stack *s)
{
	Token *popped;

	if (s->top == NULL)
		return NULL;
	popped = s->top;
	s->top = s->top->next;
	s->size--;
	return popped;
}

static void
token_free(Token *t)
{
	if (!t)
		return;
	if (t->type == TOKEN_STRING && t->str != NULL)
		free(t->str);
	free(t);
}

static void
stack_free(Stack *s)
{
	while (s->top != NULL)
		token_free(pop(s));
}

static void
stack_reverse(Stack *s)
{
	Stack tmp = { .top = NULL, .size = 0 };
	Token *t;
	while ((t = pop(s)) != NULL) push(&tmp, t);
	while ((t = pop(&tmp)) != NULL) push(s, t);
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	Stack stack, temp_stack;
	Token *t;

	stack.top = NULL;
	stack.size = 0;

	if (argc > 1) {
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			fprintf(stderr, "Could not open file '%s'.\n", argv[1]);
			return EXIT_FAILURE;
		}
	} else {
		fp = stdin;
	}

	parse_until(fp, &stack, NULL);

	if (fp != stdin)
		fclose(fp);

#if DEBUG
	fprintf(stderr, "\nFinal stack contents:\n");
	temp_stack.top = NULL;
	temp_stack.size = 0;

	while (stack.top != NULL) {
		t = pop(&stack);
		push(&temp_stack, t);
	}
	stack_reverse(&stack);

	while (stack.top != NULL) {
		t = pop(&stack);
		if (t->type == TOKEN_STRING) {
			fprintf(stderr, "String: '%s'\n", t->str);
		} else if (t->type == TOKEN_NUMBER) {
			fprintf(stderr, "Number: %g\n", t->num);
		} else if (t->type == TOKEN_FUNCTION) {
			fprintf(stderr, "Function: \n");
		}
		token_free(t);
	}
#endif

	return 0;
}
