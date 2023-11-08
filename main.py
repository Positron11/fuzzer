import os
import json
# import subprocess
from compiler import gen_header_src, gen_main_src

os.makedirs("./build", exist_ok=True) 

filename = "fuzzing"
max_depth = 100

with open("grammar.json", "r") as f:
	grammar = json.load(f)

with open(f"build/{filename}.h", "w") as f:
	header = gen_header_src(grammar)
	print(header, file=f)

with open(f"build/{filename}.c", "w") as f:
	source = gen_main_src(grammar, filename)
	print(source, file=f)

# # run this segment to create, compile, and run test driver c code
# with open("build/main.c", "w") as f:	
# 	print( "#include <stdlib.h>\n"											\
# 		  f"#include \"{filename}.h\"\n\n"									\
# 			"int main(int argc, char const *argv[]) {\n"					\
# 			"\tint max_depth = argc > 1 ? strtod(argv[1], 0) : 10;\n"		\
# 			"\tfuzz(max_depth);\n"											\
# 			"\treturn 0;\n"													\
# 			"}\n", file=f)

# subprocess.run(f"gcc -o build/fuzzer build/main.c build/{filename}.c", shell=True)									
# subprocess.run(f"./build/fuzzer {max_depth}", shell=True)									
