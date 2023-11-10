def nonterminal(token):
	return token[0] == "<" and token[-1] == ">"


def nonterminals(rule):
	return [token for token in rule if nonterminal(token)]


# identify vicious token rings
def reprehensible(rule, key, grammar, visited=None):
	if visited is None: visited = [key]
	nonterms = nonterminals(rule)

	# if rule consists entirely of terminals (ie. the path has terminated) mark as non-reprehensible
	if not nonterms: return False

	# visited[0] in rule -> rule loops back to origin
	# visited[-1] in rule -> rule is recursive with respect to itself
	if visited[0] in rule or visited[-1] in rule: return True

	for token in nonterms:
		# if a loop occurs partway through the expansion path, set the origin to the beginning of this new loop
		if token in visited: visited = visited[visited.index(token):-1]
		else: visited.append(token)

		for rule in grammar[token]:
			if not reprehensible(rule, token, grammar, visited): return False

	# if there are no non-reprehensible paths found, mark the rule as reprehensible
	return True	


# replace terminal tokens with ascii codes
def byteify(grammar):
	new_grammar = dict()
	
	for definition in grammar:
		new_grammar[definition] = [[token if nonterminal(token) else ord(token) for token in rule] for rule in grammar[definition]]

	return new_grammar


# filter out reprehensible rules
def cheapen(grammar):
	new_grammar = dict();	
	
	for key in grammar:
		new_grammar[key] = [rule for rule in grammar[key] if not reprehensible(rule, key, grammar)]
	
	return new_grammar


# filter out non-reprehensible rules
def upscale(grammar):
	new_grammar = dict();	
	
	for key in grammar:
		if filtered_rules := [rule for rule in grammar[key] if reprehensible(rule, key, grammar)]:
			new_grammar[key] = filtered_rules
	
	return new_grammar


# convert token to valid function name segment
def sanitize(token):
	return token[1:-1].replace('-', '_')


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


# generate static source
def gen_static_src(header):
	return "#include <stdio.h>\n"								\
		   "#include <stdlib.h>\n"								\
		   "#include <time.h>\n"								\
		  f"#include \"{header}.h\"\n\n"						\
		   "void fuzz(int min_depth, int max_depth) {\n"		\
		   "\tsrand((unsigned) time(0));\n"						\
		   "\tgen_start_rand(min_depth, max_depth, 0);\n"		\
		   "}\n"


# generate compiled fuzzer core source by cost
def gen_def_src(grammar):	
	grammars = {
		"cheap": byteify(cheapen(grammar)),
		"expensive": byteify(upscale(grammar)),
		"rand": byteify(grammar)
	}

	out = str()

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


# aggregate compiled fuzzer source
def gen_main_src(grammar, header):
	out = gen_static_src(header) + "\n"
	out += gen_def_src(grammar)

	return out