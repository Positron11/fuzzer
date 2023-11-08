import json
import types
from compiler import gen_fuzz_src as compile

with open("grammar.json", "r") as f:
	grammar = json.load(f)

source = compile(grammar)
fuzzer = types.ModuleType("fuzzer")
exec(source, fuzzer.__dict__)

fuzz = fuzzer.fuzz(10)
print(fuzz)

