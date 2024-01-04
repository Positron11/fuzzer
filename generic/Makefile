build/:
	mkdir build

build/_$(grammar).h: build/
	python ../utilities/tojson.py ../grammars/$(grammar).g4 > build/_$(grammar).ebnf
	python ../utilities/ebnftosimple.py build/_$(grammar).ebnf > build/_$(grammar).fbjson
	python utilities/ggc.py build/_$(grammar).fbjson > $@

iterative: fuzzers/fuzz-iter.c build/ build/_$(grammar).h
	gcc -o build/gi-fuzzer -include build/_$(grammar).h fuzzers/fuzz-iter.c
	rm build/_*

recursive: fuzzers/fuzz-recr.c build/ build/_$(grammar).h
	gcc -o build/gr-fuzzer -include build/_$(grammar).h fuzzers/fuzz-recr.c
	rm build/_*

clean:
	rm -r build/