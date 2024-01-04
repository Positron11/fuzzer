#include <stdio.h>
#include <stdlib.h>

#include "fuzzutils.h"
#include "gstruct.h"

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