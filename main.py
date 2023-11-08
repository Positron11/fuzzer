import json
import types
from compiler import gen_fuzz_src as compile

with open("grammar.json", "r") as f:
	grammar = json.load(f)

source = compile(grammar)
print(source)

