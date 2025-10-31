#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parga.h"

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

static int
quote_end(int c)
{
	return c == QUOTE_END;
}

static int
parse_until(FILE *fp, Stack *s, int (*stop_cond)(int))
{
	int c;
	Token *t;

	while ((c = fgetc(fp)) != EOF) {
		if (stop_cond && stop_cond(c))
			return c;
		if (isspace(c))
			continue;

		if ((c >= 0 && c < 128 && jumptable[c] != NULL)) {
			if ((t = jumptable[c](fp, c))) {
				push(s, t);

			#if DEBUG
				DEBUG_PRINT_TOKEN(t);
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
parse_number(FILE *fp, int consumed)
{
	int c, buf_pos, base, digit;
	bool in_fractional;
	char buffer[MAX_DIGITS], *hash_pos, *start_pos, *p;
	double result, fractional_place;
	Token *t;

	buffer[0] = consumed;
	buf_pos = 1;
	base = 10;
	result = 0.0;
	fractional_place = 1.0;
	in_fractional = false;

	t = malloc(sizeof(Token));
	if (!t)
		ERROR("Memory allocation failed");

	t->type = TOKEN_NUMBER;
	t->next = NULL;
	t->num = 0.0;

	/* Read all characters that could be part of a number */
	while ((c = fgetc(fp)) != EOF && buf_pos < MAX_DIGITS - 1) {
		if (isalnum(c) || c == '.' || c == '#') {
			buffer[buf_pos++] = c;
		} else {
			ungetc(c, fp);
			break;
		}
	}
	buffer[buf_pos] = '\0';

	/* Base specification (Erlang-style: base#number) */
	hash_pos = strchr(buffer, '#');
	start_pos = buffer;

	if (hash_pos != NULL) {
		*hash_pos = '\0'; /* Temporarily terminate base part */
		base = 0;

		for (p = buffer; *p; p++) {
			if (!isdigit(*p))
				ERROR("Invalid base");
			base = base * 10 + (*p - '0');
		}

		if (base < 2 || base > 36)
			ERROR("Base must be between 2 and 36: %d", base);

		start_pos = hash_pos + 1; /* Start parsing after '#' */
	}

	for (p = start_pos; *p; p++) {
		if (*p == '.') {
			if (in_fractional)
				ERROR("Multiple decimal points in number");
			in_fractional = true;
			continue;
		}

		digit = char_to_digit(*p, base);
		if (digit == -1)
			ERROR("Invalid digit for base %d: %c", base, *p);

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
parse_hyphen(FILE *fp, int consumed)
{
    int next = fgetc(fp);
    if (isdigit(next)) {
        Token *t = parse_number(fp, next);
        if (t) t->num = -t->num;
        return t;
    }
    ungetc(next, fp);
    return parse_operator(NULL, consumed);
}

static Token *
parse_string(FILE *fp, int consumed)
{
	UNUSED(consumed);
	int c, i;
	char buffer[MAX_STRING_LEN];
	Token *t;

	i = 0;

	while ((c = fgetc(fp)) != EOF && c != STR_DELIM && i < MAX_STRING_LEN - 1)
		buffer[i++] = c;

	if (c != STR_DELIM)
		ERROR("Unterminated string literal");

	buffer[i] = '\0';

	t = malloc(sizeof(Token));
	if (!t)
		ERROR("Memory allocation failed");

	t->type = TOKEN_STRING;
	t->next = NULL;
	t->str = malloc(i + 1);
	if (!t->str) {
		free(t);
		ERROR("Memory allocation failed");
	}

	strcpy(t->str, buffer);
	return t;
}

static Token *
parse_symbol(FILE *fp, int consumed)
{
	int c, i;
	char buffer[MAX_DIGITS];
	Token *t;

	i = 1;
	buffer[0] = consumed;

	while (isalpha((c = fgetc(fp))) && i < MAX_DIGITS - 1)
		buffer[i++] = c;

	if (c != EOF) ungetc(c, fp);

	buffer[i] = '\0';

	t = malloc(sizeof(Token));
	if (!t)
		ERROR("Memory allocation failed");

	t->type = TOKEN_SYMBOL;
	t->next = NULL;
	t->str = malloc(i + 1);
	if (!t->str) {
		free(t);
		ERROR("Memory allocation failed");
	}

	strcpy(t->str, buffer);
	return t;
}

static Token *
parse_operator(FILE *fp, int consumed)
{
	UNUSED(fp);
	Token *t;

	t = malloc(sizeof(Token));
	if (!t)
		ERROR("Memory allocation failed");

	t->type = TOKEN_SYMBOL;
	t->next = NULL;
	t->str = malloc(2);
	if (!t->str) {
		free(t);
		ERROR("Memory allocation failed");
	}
	t->str[0] = consumed;
	t->str[1] = '\0';

	return t;
}

static Token *
parse_quote(FILE *fp, int consumed)
{
	UNUSED(consumed);
	Token *t;
	Stack *s;

	t = malloc(sizeof(Token));
	if (!t)
		ERROR("Memory allocation failed");

	t->type = TOKEN_QUOTE;
	t->next = NULL;

	s = malloc(sizeof(Stack));
	if (!s) {
		free(t);
		ERROR("Memory allocation failed");
	}
	s->top = NULL;
	s->size = 0;

	if (parse_until(fp, s, quote_end) != QUOTE_END)
		ERROR("Unterminated quote");

	t->stack = s;
	return t;
}

static Token *
parse_effect(FILE *fp, int consumed)
{
	UNUSED(consumed);
	int c;
	while ((c = fgetc(fp)) != EOF && c != EFFECT_END);

	return NULL;
}

static Token *
parse_comment(FILE *fp, int consumed)
{
	UNUSED(consumed);
	int c;
	while ((c = fgetc(fp)) != EOF && c != COMMENT_DELIM);

	return NULL;
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
	if (!t) return;

	switch (t->type) {
	case TOKEN_SYMBOL:
	case TOKEN_STRING: free(t->str); break;
	case TOKEN_QUOTE: stack_free(t->stack); free(t->stack); break;
	}
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
	Stack stack;

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
	fprintf(stderr, "\nStack contents (top to bottom):\n");
	for (Token *t = stack.top; t; t = t->next) DEBUG_PRINT_TOKEN(t);
#endif

	stack_free(&stack);

	return 0;
}
