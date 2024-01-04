#include <stdio.h>
#include <stdlib.h>

#include "fuzzer.h"
#include "gstruct.h"
#include "fuzzutils.h"

void fuzzer(Grammar* grammar, depth_t max_depth, depth_t depth, token_t token);

// wrapper function
void fuzz(Grammar* grammar, depth_t max_depth) {
	fuzzer(grammar, max_depth, 0, START);
}

// main fuzzer function
void fuzzer(Grammar* grammar, depth_t max_depth, depth_t current_depth, token_t token) {
	Definition* definition = &grammar->definitions[token - START];
	Rule* rule = get_rule(definition, current_depth >= max_depth);
	
	for (size_t i = 0; i < rule->token_count; i++) {
		if (rule->tokens[i] >= 0) putchar(rule->tokens[i]); // if token is terminal print to stdout
		else fuzzer(grammar, max_depth, current_depth + 1, rule->tokens[i]); // otherwise fuzz token
	}
}
