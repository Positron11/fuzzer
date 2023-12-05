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

	# if looping back onto expansion path, mark as reprehensible
	if set(nonterms).intersection(set(visited)): return True
		
	# if any non-reprehensible continuation pathways found, mark as non-reprehensible
	for token in nonterms:
		for rule in grammar[token]:
			if not reprehensible(rule, token, grammar, visited + [token]): return False

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