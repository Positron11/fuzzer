import json
import subprocess
from compiler import gen_fuzz_src as compile

filename = "fuzzer"
max_depth = 100

with open("grammar.json", "r") as f:
	grammar = json.load(f)

with open(f"{filename}.c", "w") as f:
	source = compile(grammar)
	print(source, file=f)

subprocess.run(f"gcc -o {filename}.o {filename}.c", shell=True)
subprocess.run(f"./{filename}.o {max_depth}", shell=True)



