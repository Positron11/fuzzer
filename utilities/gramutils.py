def nonterminal(token):
	return token[0] == "<" and token[-1] == ">"


def nonterminals(rule):
	return [token for token in rule if nonterminal(token)]


# identify token rings
def ostracize(grammar, key="<start>", visited=["<start>"], flagged=[]):
	for rule in grammar[key]:
		if f"{key, rule}" not in flagged:
			nonterms = set(nonterminals(rule))

			# if looping back onto expansion path, flag rule
			if set(nonterms).intersection(set(visited)):
				flagged.append(f"{key, rule}")
			
			else: # otherwise continue down expansion paths 
				for token in nonterms:
					ostracize(grammar, token, visited.copy() + [token], flagged)

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
	flagged = ostracize(grammar)

	new_grammar = dict();	
	
	for key in grammar:
		new_grammar[key] = [rule for rule in grammar[key] if f"{key, rule}" not in flagged]
	
	return new_grammar


# generate sorted grammar
def sort(grammar):
	cheap_grammar = cheapen(grammar)

	new_grammar = dict();	
	
	for key in grammar:
		new_grammar[key] = list()

		# add cheap rules first...
		for rule in grammar[key]:
			if rule in cheap_grammar[key]: new_grammar[key].append(rule)

		# ...and then add expensive rules
		for rule in grammar[key]:
			if rule not in cheap_grammar[key]: new_grammar[key].append(rule)
	
	return new_grammar


# convert token to valid function name segment
def sanitize(token):
	return token[1:-1].replace('-', '_')
