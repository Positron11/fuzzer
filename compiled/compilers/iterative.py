from utils.gramutils import cheapen, sort, byteify, sanitize


# generate header source
def gen_header_src(grammar):
	# generate static header source
	out = """#ifndef GRAMMAR_H_INCLUDED
#define GRAMMAR_H_INCLUDED

typedef void (*func)();

typedef struct Lambda {
	int args[2];
	func func;
} Lambda;

void fuzz(int seed, int max_depth);\n\n"""

	# generate generator function declarations
	for token in grammar:
		out += f"void gen_{sanitize(token)}_cheap();\n"
		out += f"void gen_{sanitize(token)}_rand(int max_depth, int depth);\n\n"
			   
	return out + "void write(int token);\n\n"	\
				 "#endif\n"


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
#include <string.h>
#include \"{header}.h\"

Lambda stack[1024];
int stack_len = 1;

void fuzz(int seed, int max_depth) {{
	srand((unsigned) seed);
	
	stack[0] = (Lambda) {{.args={{max_depth, 0}}, .func=&gen_start_rand}};
	while (stack_len > 0) stack[0].func(stack[0].args[0], stack[0].args[1]);
	
	return;
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
				out +=  "\tif (depth >= max_depth) {\n"													\
					   f"\t\tstack[0] = (Lambda) {{.args={{}}, .func=&gen_{sanitize(key)}_cheap}};\n"	\
						"\t\treturn;\n"																	\
						"\t}\n\n"

			# generate random value
			out += f"\tswitch(rand() % {rule_count}) {{"

			# generate rule choices
			for i, rule in enumerate(grammars[cost][key]):
				out += f"\n\t\tcase {i}:\n"

				if len(rule) != 1:
					out += f"\t\t\tmemmove(stack + {len(rule)}, stack + 1, stack_len * sizeof(Lambda));\n"

				# generate token expressions - write directly to stdout if
				# terminal, otherwise link to corresponding generator function
				for i, token in enumerate(rule):
					out += f"\t\t\tstack[{i}] = "
					
					if isinstance(token, int):
						out += f"(Lambda) {{.args={{{token}}}, .func=&write}};\n"
					elif cost == "cheap":
						out += f"(Lambda) {{.args={{}}, .func=&gen_{sanitize(token)}_cheap}};\n"
					else:
						out += f"(Lambda) {{.args={{max_depth, depth + 1}}, .func=&gen_{sanitize(token)}_rand}};\n"
				
				if len(rule) != 1:
					out += f"\t\t\tstack_len += {len(rule) - 1};\n"

				out +=  "\t\t\treturn;\n"

			out += "\t}\n"	\
				   "}\n\n"

	# manually create write function
	return out + """void write(int token) {
	memmove(stack, stack + 1, --stack_len * sizeof(Lambda));
	putchar(token);
	return;
}"""
