#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define START_TOKEN INT_MIN

// expansion cost definitions
#define LOW_COST 0
#define HIGH_COST 1
#define RAND_COST rand() % 2

// rule structure
typedef struct Rule {
	size_t const token_count;
	int const* tokens;
} Rule;

// definition structure
typedef struct Definition {
	char const* name;
	size_t const rule_count[2];
	Rule const* rules[2]; // [0]: cheap rules, [1]: costly rules
} Definition;

// grammar structure
typedef struct Grammar {
	size_t const def_count;
	Definition const* definitions;
} Grammar;

// function declarations
void fuzzer(Grammar* grammar, unsigned int min_depth, unsigned int max_depth, unsigned int depth, int const* tokens, int token_count);
Rule const* get_rule(Grammar* grammar, int def, int cost);

int main(int argc, char const *argv[]) {
	srand((unsigned) time(0)); // initialize random

	// define grammar
	enum nonterminals {
		start = START_TOKEN, 
		phone, 
		area, 
		number, 
		digit,
	};
	Grammar grammar = { .def_count=5, .definitions=(Definition []) {
		(Definition) { .name="start", .rule_count={1, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(int const[]) {phone} }
			}
		} },
		(Definition) { .name="phone", .rule_count={2, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=3, .tokens=(int const[]){number, '-', number} },
				(Rule) { .token_count=4, .tokens=(int const[]){area, number, '-', number} }
			}
		} },
		(Definition) { .name="area", .rule_count={1, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=5, .tokens=(int const[]){'(', '+', digit, digit, ')'} }
			}
		} },
		(Definition) { .name="number", .rule_count={1, 1}, .rules={
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(int const[]){digit} },
			},
			(Rule []) {
				(Rule) { .token_count=2, .tokens=(int const[]){number, digit} }
			}
		} },
		(Definition) { .name="digit", .rule_count={10, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(int const[]){'0'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'1'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'2'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'3'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'4'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'5'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'6'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'7'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'8'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'9'} },
			}
		} }
	} };

	// set options from command line if possible otherwise default
	unsigned int min_depth = argc > 1 ? strtod(argv[1], 0) : 2;
	unsigned int max_depth = argc > 1 ? (argc > 2 ? strtod(argv[2], 0) : min_depth) : 4;
	unsigned int runs = argc > 3 ? strtod(argv[3], 0) : 1;
	
	// main loop
	for (size_t i = 0; i < runs; i++) {
		fuzzer(&grammar, min_depth, max_depth, 0, (int const[]) {START_TOKEN}, 1);
	}

	return EXIT_SUCCESS;
}

// fuzzer function
void fuzzer(Grammar* grammar, unsigned int min_depth, unsigned int max_depth, unsigned int depth, int const* tokens, int token_count) {
	int token = tokens[0]; // get token

	if (token >= 0) { // if token is terminal...
		putchar(token); // print to stdout

	} else { // ...otherwise if token is nonterminal
		// calculate cost based on depth and force low if def not recursive
		unsigned int cost = depth < min_depth ? HIGH_COST : (depth >= max_depth ? LOW_COST : RAND_COST);		
		cost = (((grammar->definitions)[-(START_TOKEN - token)]).rule_count)[1] ? cost : LOW_COST;
		
		// get rule and fuzz
		Rule const* rule = get_rule(grammar, token, cost);
		fuzzer(grammar, min_depth, max_depth, depth + 1, rule->tokens, rule->token_count);
	}
	
	// if buffer exists, get buffer and fuzz
	int buffer_len = token_count - 1;
	if (buffer_len) {
		int buffer[buffer_len];
		memcpy(buffer, tokens + 1, buffer_len*sizeof(int));
		fuzzer(grammar, min_depth, max_depth, depth, buffer, buffer_len);
	}
}

// get rule by key
Rule const* get_rule(Grammar* grammar, int def, int cost) {
	Definition definition = (grammar->definitions)[-(START_TOKEN - def)];
	return &((definition.rules)[cost])[rand() % (definition.rule_count)[cost]];
}
