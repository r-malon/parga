#ifndef PARGA_H
#define PARGA_H

#define QUOTE_START '['
#define QUOTE_END ']'
#define EFFECT_START '('
#define EFFECT_END ')'
#define STR_DELIM '\''
#define COMMENT_DELIM '\\'
#define ERROR(msg) do { fputs(msg "\n", stderr); exit(EXIT_FAILURE); } while (0)
#define UNUSED(x) (void)(x)

#ifndef MAX_STRING_LEN
#define MAX_STRING_LEN 1024
#endif

#ifndef MAX_DIGITS
#define MAX_DIGITS 64
#endif

#define DEBUG_PRINT_TOKEN(t) \
	switch ((t)->type) { \
	case TOKEN_STRING:   fprintf(stderr, "String: '%s'\n", (t)->str); break; \
	case TOKEN_NUMBER:   fprintf(stderr, "Number: %g\n", (t)->num); break; \
	case TOKEN_QUOTE:    fprintf(stderr, "Function\n"); break; \
	case TOKEN_SYMBOL:   fprintf(stderr, "Symbol: '%s'\n", (t)->str); break; \
	}

enum {
	TOKEN_STRING = 'A',
	TOKEN_NUMBER = '0',
	TOKEN_QUOTE = 'q',
	TOKEN_SYMBOL = 's',
};

typedef struct Token Token;
typedef struct Stack Stack;
typedef Token *(*Parser)(FILE *, int);

struct Stack {
	Token *top;
	size_t size;
};

struct Token {
	int type;
	Token *next;
	union {
		char *str;
		double num;
		Stack *stack;
	};
};

static int parse_until(FILE *, Stack *, int (*)(int));
static Token *parse_number(FILE *, int);
static Token *parse_hyphen(FILE *, int);
static Token *parse_comment(FILE *, int);
static Token *parse_string(FILE *, int);
static Token *parse_symbol(FILE *, int);
static Token *parse_operator(FILE *, int);
static Token *parse_quote(FILE *, int);
static Token *parse_effect(FILE *, int);
static int char_to_digit(int, int);
static void push(Stack *, Token *);
static Token *pop(Stack *);
static void token_free(Token *);
static void stack_free(Stack *);

static Parser jumptable[128] = {
	['0' ... '9'] = parse_number,
	['a' ... 'z'] = parse_symbol,
	['A' ... 'Z'] = parse_symbol,
	[STR_DELIM] = parse_string,
	[COMMENT_DELIM] = parse_comment,
	[QUOTE_START] = parse_quote,
	[EFFECT_START] = parse_effect,
	['+'] = parse_operator,
	['-'] = parse_hyphen,
	['*'] = parse_operator,
	['/'] = parse_operator,
	['%'] = parse_operator,
	['='] = parse_operator,
	['<'] = parse_operator,
	['>'] = parse_operator,
	['!'] = parse_operator,
	['&'] = parse_operator,
	['|'] = parse_operator,
	['^'] = parse_operator,
	['~'] = parse_operator,
};

#endif /* PARGA_H */
