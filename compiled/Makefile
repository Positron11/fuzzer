help:
	@echo "Usage: make [iterative|recursive] grammar=[name]"

iterative: build/ci_fuzz-$(grammar)
recursive: build/cr_fuzz-$(grammar)

build/:
	mkdir build

build/c%_fuzz-$(grammar): main.py build/
	python main.py $* ../grammars/$(grammar).json
	gcc -std=gnu89 -o build/c$*_fuzz-$(grammar) build/_driver.c build/_core.c
	rm build/_*

clean:
	rm -r build/

.PHONY: help iterative recursive clean