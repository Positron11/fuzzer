import json


# generate grammar from input file
def load_grammar(f):
	data = json.load(f)
	gstring = json.dumps(data["[grammar]"])

	gstring = gstring.replace("[],", "[\"\"],") # populate empty rules
	gstring = gstring.replace(data["[start]"], "<start>") # normalize start key

	return json.loads(gstring)


# determine if a rule is nonterminal
def nonterminal(token):
	return token[0] == "<" and token[-1] == ">" if token else False


# get all nonterminals in rule
def nonterminals(rule):
	return [token for token in rule if nonterminal(token)]


# convert token to valid function name segment
def sanitize(token):
	return token[1:-1].replace('-', '_')


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
	return flagged


# calculate cost of symbol based on expansions
def symbol_cost(grammar, symbol, seen, cache):
	if symbol in seen: return float("inf")
	expansion_costs = []
	
	for rule in grammar.get(symbol, []):
		if symbol in cache and str(rule) in cache[symbol]:
			expansion_costs.append(cache[symbol][str(rule)])
		
		else:
			expansion_costs.append(expansion_cost(grammar, rule, seen | {symbol}, cache))
	
	return min(expansion_costs, default=0)


# calculate cost of expansion
def expansion_cost(grammar, tokens, seen, cache):
	return max((symbol_cost(grammar, token, seen, cache) for token in tokens if token in grammar), default=0) + 1


# calculate cost for each rule in grammar
def compute_cost(grammar):
	rule_cost = {}
	
	for k in grammar:
		rule_cost[k] = {}
	
		for rule in grammar[k]:
			rule_cost[k][str(rule)] = expansion_cost(grammar, rule, set(), rule_cost)
	
	return rule_cost


# filter out reprehensible rules
def cheapen(grammar):
	new_grammar = dict()
	
	# remove all flagged rules
	flagged = ostracize(grammar)

	for key in grammar:
		new_grammar[key] = [rule for rule in grammar[key] if f"{key, rule}" not in flagged]
	
	# re-populate empty keys with minimum cost rules
	empty_keys = [key for key in new_grammar if len(new_grammar[key]) == 0]

	if empty_keys:
		costs = compute_cost(grammar)

		for key in empty_keys:	
			min_cost = min(costs[key].values())
			cheap_rules = [rule for rule in costs[key] if costs[key][rule] == min_cost]
			new_grammar[key] = [rule.translate({ord(c): None for c in "['']"}).split(", ") for rule in cheap_rules]

	return new_grammar


# replace terminal tokens with ascii codes
def byteify(grammar):
	new_grammar = dict()
	
	for key in grammar:
		new_grammar[key] = list()

		for rule in grammar[key]:
			# convert nonterminal strings to list of cahracter ascii codes (null if empty rule/token)
			new_rule = [[token] if nonterminal(token) else [ord(c) for c in list(token)] if len(token) else [0] for token in rule]
			
			# flatten and append token list
			new_grammar[key].append([token for token_list in new_rule for token in token_list])

	return new_grammar


# generate sorted grammar
def sort(grammar, cheap_grammar):
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
