import sys
sys.path.append("../..")

import json
from utilities.gramutils import cheapen, upscale, byteify, sanitize

with open(sys.argv[1], "r") as f:
	grammar = json.load(f)

grammars = {
	"cheap": byteify(cheapen(grammar)),
	"expensive": byteify(upscale(grammar)),
	"rand": byteify(grammar)
}

out = """#ifndef CSLIB_H_INCLUDED
#define CSLIB_H_INCLUDED

#include <stdio.h>
#include <limits.h>

typedef signed char token_t;

typedef struct Rule {
	size_t token_count;
	token_t* tokens;
} Rule;

typedef struct Definition {
	size_t rule_count[2];
	Rule* rules[2]; // [0]: cheap, [1]: costly
} Definition;

typedef struct Grammar {
	Definition* definitions;
} Grammar;\n\n"""

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