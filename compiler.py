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


# convert token to valid function name segment
def sanitize(token):
	return token[1:-1].replace('-', '_')


# generate compiled fuzzer core source by cost
def gen_def_src(key, grammar, cheap=False):
	if cheap: grammar = cheapen(grammar)
	grammar = byteify(grammar)
	rule_count = len(grammar[key])

	
	if cheap:
		out = f"void gen_{sanitize(key)}_cheap() {{\n"
	else:
		out = f"void gen_{sanitize(key)}(int max_depth, int depth) {{\n"		\
			   "\tif (depth > max_depth) {\n"									\
			  f"\t\tgen_{sanitize(key)}_cheap();\n"								\
			   "\t\treturn;\n"													\
			   "\t}\n\n"
			  
	out += f"\tint val = rand() % {rule_count};\n"
		
	for i in range(rule_count):
		out += f"\n\tif (val == {i}) {{\n"
		
		for token in grammar[key][i]:
			if isinstance(token, int):
				out += f"\t\tputchar({token});\n"
			else:
				if cheap: out += f"\t\tgen_{sanitize(token)}_cheap();\n"
				else: out += f"\t\tgen_{sanitize(token)}(max_depth, depth + 1);\n"

		out += "\t\treturn;\n"		\
			   "\t}\n"

	return out + "}\n"


# generate static source
def gen_static_init():
	return "#include <stdio.h>\n"		\
		   "#include <stdlib.h>\n"		\
		   "#include <time.h>\n"
		   

# generate static source
def gen_def_init(grammar):
	out = str()

	for token in grammar:
		out += f"void gen_{sanitize(token)}_cheap();\n"							\
			   f"void gen_{sanitize(token)}(int max_depth, int depth);\n"		\
			   
	return out


# generate static source
def gen_driver_src():
	return "int main(int argc, char const *argv[]) {\n"					\
		   "\tsrand((unsigned) time(0));\n"								\
		   "\tint max_depth = argc > 1 ? strtod(argv[1], 0): 10;\n"		\
		   "\tgen_start(max_depth, 0);\n"								\
		   "\treturn 0;\n"												\
		   "}\n"


# aggregate compiled fuzzer source
def gen_fuzz_src(grammar):
	out = f"{gen_static_init()}\n"
	out += f"{gen_def_init(grammar)}\n"
	out += f"{gen_driver_src()}\n"

	for definition in grammar:
		out += f"{gen_def_src(definition, grammar, cheap=True)}\n"		\
			   f"{gen_def_src(definition, grammar)}\n"

	return out
