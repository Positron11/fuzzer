from utilities.gramutils import cheapen, sort, byteify, sanitize


# generate header source
def gen_header_src(grammar):
	# generate static header source
	out = """#ifndef GRAMMAR_H_INCLUDED
#define GRAMMAR_H_INCLUDED

void fuzz(int seed, int max_depth);\n\n"""

	# generate generator function declarations
	for token in grammar:
		out += f"void gen_{sanitize(token)}_cheap();\n"		
		out += f"void gen_{sanitize(token)}_rand(int max_depth, int depth);\n\n"
			   
	return out + "\n#endif\n"


# generate compiled fuzzer core source
def gen_main_src(grammar, header):	
	cheap_grammar = cheapen(grammar)

	grammars = {
		"cheap": byteify(cheap_grammar),
		"rand": byteify(sort(grammar, cheap_grammar))
	}

	# generate static source
	out = f"""#include <stdio.h>
#include <stdlib.h>
#include \"{header}.h\"

void fuzz(int seed, int max_depth) {{
	srand((unsigned) seed);
	gen_start_rand(max_depth, 0);
}}\n\n"""

	# generate generator function definitions
	for key in grammar:
		for cost in grammars.keys():
			try: rule_count = len(grammars[cost][key])
			except: continue
			
			# generate main function wrapper
			out += f"void gen_{sanitize(key)}_{cost}({'int max_depth, int depth' if cost != 'cheap' else ''}) {{\n"
			
			if cost == "rand":
				# use cheap function if past max depth
				out +=  "\tif (depth >= max_depth) {\n"			\
					   f"\t\tgen_{sanitize(key)}_cheap();\n"	\
						"\t\treturn;\n"							\
						"\t}\n\n"

			# generate random value
			out += f"\tswitch(rand() % {rule_count}) {{"

			# generate rule choices
			for i, rule in enumerate(grammars[cost][key]):
				out += f"\n\t\tcase {i}:\n"

				# generate token expressions - write directly to stdout if
				# terminal, otherwise link to corresponding generator function
				for token in rule:
					if isinstance(token, int):
						out += f"\t\t\tputchar({token});\n"
					elif cost == "cheap":
						out += f"\t\t\tgen_{sanitize(token)}_cheap();\n"
					else:
						out += f"\t\t\tgen_{sanitize(token)}_rand(max_depth, depth + 1);\n"
				
				out +=  "\t\t\treturn;\n"

			out += "\t}\n"	\
				   "}\n\n"

	return out
