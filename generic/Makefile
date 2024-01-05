help:
	@echo "Usage: make [iterative|recursive] grammar=[name]"

iterative: build/gi_fuzz-$(grammar)
recursive: build/gr_fuzz-$(grammar)

build/:
	mkdir build

build/_$(grammar).h: build/
	python utilities/ggc.py ../grammars/$(grammar).json > $@

build/_driver.o build/_fuzzutils.o: build/ build/_$(grammar).h
	gcc -c src/fuzzutils.c -o build/_fuzzutils.o
	gcc -c driver.c -o build/_driver.o -include build/_$(grammar).h

build/gi_fuzz-$(grammar): src/fuzz-iter.c build/ build/_driver.o build/_fuzzutils.o
	gcc -c src/fuzz-iter.c -o build/_fuzz-iter.o
	gcc -o build/gi_fuzz-$(grammar) build/_driver.o build/_fuzz-iter.o build/_fuzzutils.o
	rm build/_*

build/gr_fuzz-$(grammar): src/fuzz-recr.c build/ build/_driver.o build/_fuzzutils.o
	gcc -c src/fuzz-recr.c -o build/_fuzz-recr.o
	gcc -o build/gr_fuzz-$(grammar) build/_driver.o build/_fuzz-recr.o build/_fuzzutils.o
	rm build/_*

clean:
	rm -r build/

.PHONY: help iterative recursive clean