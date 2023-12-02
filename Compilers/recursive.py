from gramutils import cheapen, upscale, byteify, sanitize

# generate header source
def gen_header_src(grammar):
	out = "#ifndef CSLIB_H_INCLUDED\n"					\
		  "#define CSLIB_H_INCLUDED\n\n"				\
		  "void fuzz(int min_depth, int max_depth);\n\n"
	
	expensive_grammar = upscale(grammar)

	for token in grammar:
		out += f"void gen_{sanitize(token)}_cheap();\n"

		if token in expensive_grammar: 
			out += f"void gen_{sanitize(token)}_expensive(int min_depth, int max_depth, int depth);\n"
		
		out += f"void gen_{sanitize(token)}_rand(int min_depth, int max_depth, int depth);\n\n"
			   
	return out + "\n#endif\n"


# generate compiled fuzzer core source
def gen_main_src(grammar, header):	
	grammars = {
		"cheap": byteify(cheapen(grammar)),
		"expensive": byteify(upscale(grammar)),
		"rand": byteify(grammar)
	}

	out = "#include <stdio.h>\n"								\
		   "#include <stdlib.h>\n"								\
		   "#include <time.h>\n"								\
		  f"#include \"{header}.h\"\n\n"						\
		   "void fuzz(int min_depth, int max_depth) {\n"		\
		   "\tsrand((unsigned) time(0));\n"						\
		   "\tgen_start_rand(min_depth, max_depth, 0);\n"		\
		   "}\n\n"

	for key in grammar:
		for cost in grammars.keys():
			try: rule_count = len(grammars[cost][key])
			except: continue
			
			# generate main function wrapper
			out += f"void gen_{sanitize(key)}_{cost}({'int min_depth, int max_depth, int depth' if cost != 'cheap' else ''}) {{\n"
			
			if cost == "rand":
				# use cheap function if past max depth
				out +=  "\tif (depth >= max_depth) {\n"			\
					   f"\t\tgen_{sanitize(key)}_cheap();\n"	\
						"\t\treturn;\n"							\
						"\t}"

				# use expensive function (if exists) if below min depth
				if key in grammars["expensive"]:
					out +=  " else if (depth < min_depth) {\n"										\
						   f"\t\tgen_{sanitize(key)}_expensive(min_depth, max_depth, depth + 1);\n"		\
						    "\t\treturn;\n"															\
						    "\t}"
							
				out += "\n\n"

			# generate random value
			out += f"\tint val = rand() % {rule_count};\n"

			# generate rule choices
			for i, rule in enumerate(grammars[cost][key]):
				out += f"\n\tif (val == {i}) {{\n"

				# generate token expressions - write directly to stdout if
				# terminal, otherwise link to corresponding generator function
				for token in rule:
					if isinstance(token, int):
						out += f"\t\tputchar({token});\n"
					elif cost == "cheap":
						out += f"\t\tgen_{sanitize(token)}_cheap();\n"
					else:
						out += f"\t\tgen_{sanitize(token)}_rand(min_depth, max_depth, depth + 1);\n"
				
				out +=  "\t\treturn;\n"
				out += "\t}\n"

			out += "}\n\n"

	return out