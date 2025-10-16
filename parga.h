#ifndef PARGA_H
#define PARGA_H

#define FUNCTION_START '{'
#define FUNCTION_END '}'
#define STR_DELIM '\''
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
static Token *parse_number(FILE *);
static Token *parse_comment(FILE *);
static Token *parse_string(FILE *);
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

#endif /* PARGA_H */
