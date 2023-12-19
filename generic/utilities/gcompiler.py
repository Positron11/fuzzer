import sys
sys.path.append("../..")

import json
from utilities.gramutils import cheapen, byteify, sanitize

with open(sys.argv[1], "r") as f:
	grammar = json.load(f)

cheap_grammar = cheapen(grammar)
expensive_grammar = dict()
for key in grammar:
	expensive_grammar[key] = [rule for rule in grammar[key] if rule not in cheap_grammar[key]]

grammars = {
	"cheap": byteify(cheap_grammar),
	"expensive": byteify(expensive_grammar),
	"rand": byteify(grammar)
}

out = """#ifndef GRAMMAR_H_INCLUDED
#define GRAMMAR_H_INCLUDED

#include <limits.h>
#include "grammar.h"\n\n"""

out += f"enum nonterminals {{start = SCHAR_MIN, {", ".join([sanitize(token) for token in grammar][1:])}}};\n\n"

out += f"Grammar grammar = {{.definitions=(Definition []) {{\n"

for key in grammar:
	out += f"\t[{sanitize(key)} - start] = (Definition) {{.rule_count={{{len(grammars['cheap'][key])}, {len(grammars['expensive'][key]) if key in grammars['expensive'] else '0'}}}, .rules={{\n"

	for subgrammar in [grammars["cheap"], grammars["expensive"]]:
		if key in subgrammar:
			out += "\t\t(Rule []) {\n"
			for rule in subgrammar[key]: 
				tokens = ', '.join([f"'{chr(token)}'" if isinstance(token, int) else sanitize(token) for token in rule])
				out += f"\t\t\t(Rule) {{ .token_count={len(rule)}, .tokens=(token_t[]) {{{tokens}}} }},\n"
			out += "\t\t},\n"

	out += "\t} },\n"

out += f"}} }};\n\n"

print(out + "#endif");