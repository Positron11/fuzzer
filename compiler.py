import json


def print_grammar(grammar):
	for definition in grammar:
		print(definition)
		for rule in grammar[definition]:
			print(f"\t{rule}")


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
		new_grammar[definition] = [[token if token[0] == "<" and token[-1] == ">" else ord(token) for token in rule] for rule in grammar[definition]]

	return new_grammar


# filter out reprehensible rules
def cheapen(grammar):
	new_grammar = dict();	
	
	for key in grammar:
		new_grammar[key] = [rule for rule in grammar[key] if not reprehensible(rule, key, grammar)]
	
	return new_grammar


# generate compiled fuzzer core by cost
def gen_def_src(key, grammar, cheap=False):
	grammar = byteify(grammar)
	if cheap: grammar = cheapen(grammar)
	rule_count = len(grammar[key])

	if cheap:
		out = f"def gen_{key[1:-1]}_cheap():\n" 				\
			  f"\tval = random.randrange({rule_count})\n"
	else:
		out = f"def gen_{key[1:-1]}(max_depth, depth=0):\n"		\
			  f"\tif depth > max_depth:\n"						\
			  f"\t\tgen_{key[1:-1]}_cheap()\n"					\
			  f"\t\treturn\n"									\
			  f"\tval = random.randrange({rule_count})\n"
		
	for i in range(rule_count):
		out += f"\tif val == {i}:\n"
		
		for token in grammar[key][i]:
			if isinstance(token, int):
				out += f"\t\tresult.append({token})\n"
			else:
				if cheap: out += f"\t\tgen_{token[1:-1]}_cheap()\n"
				else: out += f"\t\tgen_{token[1:-1]}(max_depth, depth + 1)\n"

		out += f"\t\treturn\n"

	return out


# aggregate compiled fuzzer cores
def gen_fuzz_src(grammar):
	out = str()

	for definition in grammar:
		out += f"{gen_def_src(definition, grammar, cheap=True)}\n"	\
			   f"{gen_def_src(definition, grammar)}\n\n"

	return out


with open("grammar.json", "r") as f:
	grammar = json.load(f)

print_grammar(cheapen(grammar))


