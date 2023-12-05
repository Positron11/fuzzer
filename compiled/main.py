import sys
sys.path.append("..")

import os
import json
import subprocess
from compilers.iterative import gen_header_src, gen_main_src

# create build directory
os.makedirs("./build", exist_ok=True) 

# load grammar
with open(sys.argv[1], "r") as f:
	grammar = json.load(f)

# generate header
with open("build/core.h", "w") as f:
	header = gen_header_src(grammar)
	print(header, file=f)

# generate core
with open("build/core.c", "w") as f:
	source = gen_main_src(grammar, "core")
	print(source, file=f)

# generate driver
with open("build/driver.c", "w") as f:	
	print("""#include <stdio.h>
#include <stdlib.h>
#include "core.h"

int main(int argc, char const *argv[]) {
	if (argc <= 1) {
		puts("Error: seed argument required");
		return EXIT_FAILURE;
	}

	int seed = strtod(argv[1], 0);
	int min_depth = argc > 2 ? strtod(argv[2], 0) : 0;
	int max_depth = argc > 2 ? strtod(argv[argc - 1], 0) : 10;
	fuzz(seed, min_depth, max_depth);

	return 0;
}""", file=f)

# compile executable
subprocess.run(f"gcc -o build/fuzz build/driver.c build/core.c", shell=True)							