from utilities.gramutils import cheapen, upscale, byteify, sanitize

# generate header source
def gen_header_src(grammar):
	out = "#ifndef CSLIB_H_INCLUDED\n"						\
		  "#define CSLIB_H_INCLUDED\n\n"					\
		  "typedef void (*func)();\n\n"						\
		  "typedef struct lambda {\n"						\
		  "\tint args[3];\n"								\
		  "\tfunc func;\n"									\
		  "} lambda;\n\n"									\
		  "void fuzz(int seed, int min_depth, int max_depth);\n\n"
	
	expensive_grammar = upscale(grammar)

	for token in grammar:
		out += f"void gen_{sanitize(token)}_cheap();\n"

		if token in expensive_grammar: 
			out += f"void gen_{sanitize(token)}_expensive(int min_depth, int max_depth, int depth);\n"
		
		out += f"void gen_{sanitize(token)}_rand(int min_depth, int max_depth, int depth);\n\n"
			   
	return out + "void write(int token);\n\n"		\
				 "#endif\n"


# generate compiled fuzzer core source
def gen_main_src(grammar, header):	
	grammars = {
		"cheap": byteify(cheapen(grammar)),
		"expensive": byteify(upscale(grammar)),
		"rand": byteify(grammar)
	}

	# write tatic source and main fuzzing function
	out = "#include <stdio.h>\n"																				\
		   "#include <stdlib.h>\n"																				\
		   "#include <string.h>\n"																				\
		   "#include <time.h>\n"																				\
		  f"#include \"{header}.h\"\n\n"																		\
		   "lambda stack[1024];\n"																				\
		   "int stack_len = 1;\n\n"																				\
		   "void fuzz(int seed, int min_depth, int max_depth) {\n"												\
		   "\tsrand((unsigned) seed);\n\n"																		\
		   "\tstack[0] = (lambda) {.args={min_depth, max_depth, 0}, .func=&gen_start_rand};\n"					\
		   "\twhile (stack_len > 0) stack[0].func(stack[0].args[0], stack[0].args[1], stack[0].args[2]);\n\n"	\
		   "\treturn;\n"																						\
		   "}\n\n"

	for key in grammar:
		for cost in grammars.keys():
			try: rule_count = len(grammars[cost][key])
			except: continue
			
			# generate main function wrapper
			out += f"void gen_{sanitize(key)}_{cost}({'int min_depth, int max_depth, int depth' if cost != 'cheap' else ''}) {{\n"
			
			if cost == "rand":
				# use cheap function if past max depth
				out +=  "\tif (depth >= max_depth) {\n"																				\
					   f"\t\tstack[0] = (lambda) {{.args={{}}, .func=&gen_{sanitize(key)}_cheap}};\n"								\
						"\t\treturn;\n"																								\
						"\t}"

				# use expensive function (if exists) if below min depth
				if key in grammars["expensive"]:
					out +=  " else if (depth < min_depth) {\n"																					\
						   f"\t\tstack[0] = (lambda) {{.args={{min_depth, max_depth, depth + 1}}, .func=&gen_{sanitize(key)}_expensive}};\n"	\
							"\t\treturn;\n"																										\
						    "\t}"
							
				out += "\n\n"

			# generate random value
			out += f"\tint val = rand() % {rule_count};\n"

			# generate rule choices
			for i, rule in enumerate(grammars[cost][key]):
				out += f"\n\tif (val == {i}) {{\n"

				if len(rule) != 1:
					out += f"\t\tmemmove(&stack[{len(rule)}], &stack[1], stack_len * sizeof(lambda));\n"

				# generate token expressions - write directly to stdout if
				# terminal, otherwise link to corresponding generator function
				for i, token in enumerate(rule):
					out += f"\t\tstack[{i}] = "
					
					if isinstance(token, int):
						out += f"(lambda) {{.args={{{token}}}, .func=&write}};\n"
					elif cost == "cheap":
						out += f"(lambda) {{.args={{}}, .func=&gen_{sanitize(token)}_cheap}};\n"
					else:
						out += f"(lambda) {{.args={{min_depth, max_depth, depth + 1}}, .func=&gen_{sanitize(token)}_rand}};\n"
				
				if len(rule) != 1:
					out += f"\t\tstack_len += {len(rule) - 1};\n"

				out +=  "\t\treturn;\n"
				out += "\t}\n"

			out += "}\n\n"

	# manually create write function
	return out + "void write(int token) {\n"										\
				 "\tmemmove(&stack[0], &stack[1], stack_len * sizeof(lambda));\n"	\
				 "\tputchar(token);\n"												\
				 "\tstack_len += -1;\n"												\
				 "\treturn;\n"														\
				 "}"
