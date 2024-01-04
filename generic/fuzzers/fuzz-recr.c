#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../headers/grammar.h"

typedef size_t depth_t;

void fuzzer(Grammar* grammar, depth_t max_depth, depth_t depth, token_t token);
Rule* get_rule(Definition* definition, int cost);

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		puts("Error: seed argument required");
		return EXIT_FAILURE;
	}
	
	int seed = strtod(argv[1], 0);
	srand((unsigned) seed); // initialize random

	// set options from command line arguments
	int max_depth = argc > 2 ? strtod(argv[2], 0) : 10;
	
	fuzzer(&grammar, max_depth, 0, start);

	return EXIT_SUCCESS;
}

// main fuzzer function
void fuzzer(Grammar* grammar, depth_t max_depth, depth_t current_depth, token_t token) {
	Definition* definition = &grammar->definitions[token - start];
	Rule* rule = get_rule(definition, current_depth >= max_depth);
	
	for (size_t i = 0; i < rule->token_count; i++) {
		if (rule->tokens[i] >= 0) putchar(rule->tokens[i]); // if token is terminal print to stdout
		else fuzzer(grammar, max_depth, current_depth + 1, rule->tokens[i]); // otherwise fuzz token
	}
}

// get rule from definition
Rule* get_rule(Definition* definition, int cheap) {
	size_t cheap_count = definition->rule_count[0];
	size_t exp_count = definition->rule_count[1];
	
	// force cheap if no expensive rules
	if (!exp_count) cheap = 1;

	if (cheap) {
		return &definition->rules[0][rand() % cheap_count];
	
	} else {
		// choose from cumulative rule set
		size_t choice = rand() % (cheap_count + exp_count);
		size_t cost = choice >= cheap_count;
		if (cost) choice -= cheap_count;
		return &definition->rules[cost][choice];
	}
}
