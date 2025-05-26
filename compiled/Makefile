help:
	@printf "\033[1mUsage:\033[0m make <iterative|recursive> grammar=<name>\n"

iterative: build/ci_fuzz-$(grammar)
recursive: build/cr_fuzz-$(grammar)

build/:
	mkdir build

build/c%_fuzz-$(grammar): main.py build/
	python main.py $* ../grammars/json/$(grammar).json
	gcc -std=gnu89 -o build/c$*_fuzz-$(grammar) build/_driver.c build/_core.c
	rm build/_*

clean:
	rm -r build/

.PHONY: help iterative recursive clean