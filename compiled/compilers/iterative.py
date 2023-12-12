from utilities.gramutils import cheapen, sort, byteify, sanitize

# generate header source
def gen_header_src(grammar):
	out = "#ifndef CSLIB_H_INCLUDED\n"						\
		  "#define CSLIB_H_INCLUDED\n\n"					\
		  "typedef void (*func)();\n\n"						\
		  "typedef struct Lambda {\n"						\
		  "\tint args[2];\n"								\
		  "\tfunc func;\n"									\
		  "} Lambda;\n\n"									\
		  "void fuzz(int seed, int max_depth);\n\n"

	for token in grammar:
		out += f"void gen_{sanitize(token)}_cheap();\n"
		out += f"void gen_{sanitize(token)}_rand(int max_depth, int depth);\n\n"
			   
	return out + "void write(int token);\n\n"		\
				 "#endif\n"


# generate compiled fuzzer core source
def gen_main_src(grammar, header):	
	grammars = {
		"cheap": byteify(cheapen(grammar)),
		"rand": byteify(sort(grammar))
	}

	# write tatic source and main fuzzing function
	out = "#include <stdio.h>\n"																\
		   "#include <stdlib.h>\n"																\
		   "#include <string.h>\n"																\
		   "#include <time.h>\n"																\
		  f"#include \"{header}.h\"\n\n"														\
		   "Lambda stack[1024];\n"																\
		   "int stack_len = 1;\n\n"																\
		   "void fuzz(int seed, int max_depth) {\n"												\
		   "\tsrand((unsigned) seed);\n\n"														\
		   "\tstack[0] = (Lambda) {.args={max_depth, 0}, .func=&gen_start_rand};\n"				\
		   "\twhile (stack_len > 0) stack[0].func(stack[0].args[0], stack[0].args[1]);\n\n"		\
		   "\treturn;\n"																		\
		   "}\n\n"

	for key in grammar:
		for cost in grammars.keys():
			try: rule_count = len(grammars[cost][key])
			except: continue
			
			# generate main function wrapper
			out += f"void gen_{sanitize(key)}_{cost}({'int max_depth, int depth' if cost != 'cheap' else ''}) {{\n"
			
			if cost == "rand":
				# use cheap function if past max depth
				out +=  "\tif (depth >= max_depth) {\n"																				\
					   f"\t\tstack[0] = (Lambda) {{.args={{}}, .func=&gen_{sanitize(key)}_cheap}};\n"								\
						"\t\treturn;\n"																								\
						"\t}\n\n"

			# generate random value
			out += f"\tswitch(rand() % {rule_count}) {{"

			# generate rule choices
			for i, rule in enumerate(grammars[cost][key]):
				out += f"\n\t\tcase {i}:\n"

				if len(rule) != 1:
					out += f"\t\t\tmemmove(&stack[{len(rule)}], &stack[1], stack_len * sizeof(Lambda));\n"

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
	return out + "void write(int token) {\n"										\
				 "\tmemmove(&stack[0], &stack[1], stack_len * sizeof(Lambda));\n"	\
				 "\tputchar(token);\n"												\
				 "\tstack_len += -1;\n"												\
				 "\treturn;\n"														\
				 "}"
