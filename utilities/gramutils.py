def nonterminal(token):
	return token[0] == "<" and token[-1] == ">"


def nonterminals(rule):
	return [token for token in rule if nonterminal(token)]


# identify vicious token rings
def ostracize(rule, key, grammar, visited=None, flagged=[]):
	if visited is None: visited = [key]
	nonterms = nonterminals(rule)

	# if looping back onto expansion path, flag rule
	if set(nonterms).intersection(set(visited)):
		flagged.append(f"{key, rule}")
		return
		
	# if rule not already flagged, evaluate continuation pathways
	for token in nonterms:
		for rule in grammar[token]:
			if f"{token, rule}" not in flagged:
				ostracize(rule, token, grammar, visited + [token], flagged)

	# return flagged endpoints
	return set(flagged)


# replace terminal tokens with ascii codes
def byteify(grammar):
	new_grammar = dict()
	
	for definition in grammar:
		new_grammar[definition] = [[token if nonterminal(token) else ord(token) for token in rule] for rule in grammar[definition]]

	return new_grammar


# filter out reprehensible rules
def cheapen(grammar):
	flagged = set()

	# generate list of flagged rules
	for key in grammar:
		for rule in grammar[key]:
			if f"{key, rule}" not in flagged: 
				if x := ostracize(rule, key, grammar): flagged.update(x)

	new_grammar = dict();	
	
	for key in grammar:
		new_grammar[key] = [rule for rule in grammar[key] if f"{key, rule}" not in flagged]
	
	return new_grammar


# filter out non-reprehensible rules
def upscale(grammar):
	flagged = set()

	# generate list of flagged rules
	for key in grammar:
		for rule in grammar[key]:
			if f"{key, rule}" not in flagged: 
				if x := ostracize(rule, key, grammar): flagged.update(x)

	new_grammar = dict();	
	
	for key in grammar:
		if filtered_rules := [rule for rule in grammar[key] if f"{key, rule}" in flagged]:
			new_grammar[key] = filtered_rules
	
	return new_grammar


# convert token to valid function name segment
def sanitize(token):
	return token[1:-1].replace('-', '_')
