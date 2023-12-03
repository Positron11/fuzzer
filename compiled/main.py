import os
import json
import subprocess
from compilers.iterative import gen_header_src, gen_main_src

# create build directory
os.makedirs("./build", exist_ok=True) 

# load grammar
with open("grammars/arithmetic.json", "r") as f:
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
	print("#include <stdlib.h>\n"												\
		  "#include \"core.h\"\n\n"												\
		  "int main(int argc, char const *argv[]) {\n"							\
		  "\tint min_depth = argc > 1 ? strtod(argv[1], 0) : 0;\n"				\
		  "\tint max_depth = argc > 1 ? strtod(argv[argc - 1], 0) : 10;\n"		\
		  "\tfuzz(min_depth, max_depth);\n"										\
		  "\treturn 0;\n"														\
		  "}\n", file=f)

# compile executable
subprocess.run(f"gcc -o build/fuzz build/driver.c build/core.c", shell=True)							