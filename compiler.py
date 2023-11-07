grammar = {
	"<start>": [
		["<expr>"]
	],
	"<expr>": [
		["<term>", "+", "<expr>"],
		["<term>", "-", "<expr>"],
		["<term>"]
	],
	"<term>": [
		["<factor>", "*", "<term>"],
		["<factor>", "/", "<term>"],
		["<factor>"]
	],
	"<factor>": [
		["+", "<factor>"],
		["-", "<factor>"],
		["(", "<expr>", ")"],
		["<integer>", ".", "<integer>"],
		["<integer>"]
	],
	"<integer>": [
		["<digit>", "<integer>"],
		["<digit>"]
	],
	"<digit>": [
		["0"], 
		["1"], 
		["2"], 
		["3"], 
		["4"], 
		["5"], 
		["6"], 
		["7"], 
		["8"], 
		["9"]
	]
}

def is_nonterminal(token):
	return token[0] == "<" and token[-1] == ">"


def nonterminals(rule):
	return set([token for token in rule if is_nonterminal(token)])


def byteify(grammar):
	new_grammar = dict()
	for definition in grammar:
		new_grammar[definition] = [[token if token[0] == "<" and token[-1] == ">" else ord(token) for token in rule] for rule in grammar[definition]]
	return new_grammar


def cheapen(grammar):
	new_grammar = dict();	
	for definition in grammar:
		new_grammar[definition] = [rule for rule in grammar[definition] if not definition in rule]
	return new_grammar


def generate_src_cheap(key, grammar):
	rule_count = len(grammar[key])
				  
	out = f"def gen_{key[1:-1]}_cheap():\n" \
		  f"\tval = random.randrange({rule_count})\n"
	
	for i in range(rule_count):
		out += f"\tif val == {i}:\n"
		
		for token in grammar[key][i]:
			if isinstance(token, int):
				out += f"\t\tresult.append({token})\n"
			else:
				out += f"\t\tgen_{token[1:-1]}_cheap()\n"

	return out


new_grammar = cheapen(byteify(grammar))
for key in new_grammar:
	print(f"{generate_src_cheap(key, new_grammar)}\n")
