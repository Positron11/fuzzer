#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "grammars/arithmetic.h" // change to appropriate compiled grammar

#define LOW_COST 0
#define HIGH_COST 1
#define RAND_COST rand() % 2

typedef size_t depth_t;

void fuzzer(Grammar* grammar, depth_t min_depth, depth_t max_depth, depth_t depth, token_t token);
Rule* get_rule(Definition* definition, int cost);

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		puts("Error: seed argument required");
		return EXIT_FAILURE;
	}
	
	int seed = strtod(argv[1], 0);
	srand((unsigned) seed); // initialize random

	// set options from command line if possible otherwise default
	int min_depth = argc > 2 ? strtod(argv[2], 0) : 0;
	int max_depth = argc > 2 ? strtod(argv[argc - 1], 0) : 10;
	
	fuzzer(&grammar, min_depth, max_depth, 0, start);

	return EXIT_SUCCESS;
}

// main fuzzer function
void fuzzer(Grammar* grammar, depth_t min_depth, depth_t max_depth, depth_t current_depth, token_t token) {
	Definition* definition = &grammar->definitions[token - start];

	int rule_cost = current_depth < min_depth ? HIGH_COST : (current_depth >= max_depth ? LOW_COST : RAND_COST); // calculate cost based on depth...
	rule_cost = definition->rule_count[1] ? rule_cost : LOW_COST; // ...and force low if definition not recursive
	
	Rule* rule = get_rule(definition, rule_cost);
	
	for (size_t i = 0; i < rule->token_count; i++) {
		if (rule->tokens[i] >= 0) putchar(rule->tokens[i]); // if token is terminal print to stdout
		else fuzzer(grammar, min_depth, max_depth, current_depth + 1, rule->tokens[i]); // otherwise fuzz token
	}
}

// get rule from definition
Rule* get_rule(Definition* definition, int cost) {
	return &((definition->rules)[cost])[rand() % (definition->rule_count)[cost]];
}
