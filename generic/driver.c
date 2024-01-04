#include <stdio.h>
#include <stdlib.h>

#include "include/gstruct.h"
#include "include/fuzzer.h"

// placeholder declarations so driver core compiles independently
#ifndef GRAMMAR_H_INCLUDED
Grammar grammar;
#endif

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		puts("Error: seed argument required");
		return EXIT_FAILURE;
	}
	
	int seed = strtod(argv[1], 0);
	srand((unsigned) seed); // initialize random

	// set options from command line arguments
	int max_depth = argc > 2 ? strtod(argv[2], 0) : 10;
	
	fuzz(&grammar, max_depth);

	return EXIT_SUCCESS;
}
