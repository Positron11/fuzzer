import sys
import json

sys.path.append("../")

import compilers.iterative as iter_comp
import compilers.recursive as recr_comp


# choose compiler
if sys.argv[1] == "i":
	compiler = iter_comp

elif sys.argv[1] == "r":
	compiler = recr_comp
	

# load grammar
with open(sys.argv[2], "r") as f:
	data = json.load(f)
	grammarstr = json.dumps(data["[grammar]"])
	grammar = json.loads(grammarstr.replace(data["[start]"], "<start>"))


# generate header
with open("build/_core.h", "w") as f:
	print(compiler.gen_header_src(grammar), file=f)


# generate core
with open("build/_core.c", "w") as f:
	print(compiler.gen_main_src(grammar, "_core"), file=f)


# generate driver
driver_src = """#include <stdio.h>
#include <stdlib.h>
#include "_core.h"

int main(int argc, char const *argv[]) {
	if (argc <= 1) {
		puts("Error: seed argument required");
		return EXIT_FAILURE;
	}

	int seed = strtod(argv[1], 0);
	int max_depth = argc > 2 ? strtod(argv[2], 0) : 10;
	fuzz(seed, max_depth);

	return 0;
}"""

with open("build/_driver.c", "w") as f:	
	print(driver_src, file=f)
						