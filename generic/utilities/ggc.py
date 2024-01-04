import sys
sys.path.append("../")

import json
from utilities.gramutils import cheapen, byteify, sanitize


# load and generate grammar variants
with open(sys.argv[1], "r") as f:
	data = json.load(f)
	grammarstr = json.dumps(data["[grammar]"])
	grammar = json.loads(grammarstr.replace(data["[start]"], "<start>"))

	cheap_grammar = cheapen(grammar)

	expensive_grammar = dict()
	for key in grammar:
		rules = [rule for rule in grammar[key] if rule not in cheap_grammar[key]]

		if len(rules):
			expensive_grammar[key] = rules

	subgrammars = {
		"cheap": byteify(cheap_grammar),
		"expensive": byteify(expensive_grammar),
	}


# generate static source
out = f"""#ifndef GRAMMAR_H_INCLUDED
#define GRAMMAR_H_INCLUDED

#include <limits.h>
#include "../include/gstruct.h"

enum nonterminals {{start=START, {", ".join([sanitize(token) for token in grammar][1:])}}};

Grammar grammar = {{.definitions=(Definition []) {{\n"""


# generate dynamic source
for key in grammar:
	out += f"\t[{sanitize(key)} - start] = (Definition) {{.rule_count={{{len(subgrammars['cheap'][key])}, {len(subgrammars['expensive'][key]) if key in subgrammars['expensive'] else '0'}}}, .rules={{\n"

	for subgrammar in subgrammars.values():
		if key in subgrammar:
			out += "\t\t(Rule []) {\n"
			
			for rule in subgrammar[key]: 
				tokens = ', '.join([str(token) if isinstance(token, int) else sanitize(token) for token in rule])
				out += f"\t\t\t(Rule) {{ .token_count={len(rule)}, .tokens=(token_t[]) {{{tokens}}} }},\n"
			
			out += "\t\t},\n"

	out += "\t} },\n"


# print compiled grammar source
print(out + "} };\n\n#endif");