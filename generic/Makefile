build/:
	mkdir build

build/_$(grammar).h: build/
	python ../utilities/tojson.py ../grammars/$(grammar).g4 > build/_$(grammar).ebnf
	python ../utilities/ebnftosimple.py build/_$(grammar).ebnf > build/_$(grammar).fbjson
	python utilities/ggc.py build/_$(grammar).fbjson > $@

build/_driver.o build/_fuzzutils.o: build/ build/_$(grammar).h
	gcc -c src/fuzzutils.c -o build/_fuzzutils.o
	gcc -c driver.c -o build/_driver.o -include build/_$(grammar).h

iterative: src/fuzz-iter.c build/ build/_driver.o build/_fuzzutils.o
	gcc -c src/fuzz-iter.c -o build/_fuzz-iter.o
	gcc -o build/gi_fuzz-$(grammar) build/_driver.o build/_fuzz-iter.o build/_fuzzutils.o
	rm build/_*

recursive: src/fuzz-recr.c build/ build/_driver.o build/_fuzzutils.o
	gcc -c src/fuzz-recr.c -o build/_fuzz-recr.o
	gcc -o build/gr_fuzz-$(grammar) build/_driver.o build/_fuzz-recr.o build/_fuzzutils.o
	rm build/_*

clean:
	rm -r build/