build/:
	mkdir build

build/_$(grammar).fbjson: build/
	python ../utilities/tojson.py ../grammars/$(grammar).g4 > build/_$(grammar).ebnf
	python ../utilities/ebnftosimple.py build/_$(grammar).ebnf > build/_$(grammar).fbjson

iterative: main.py build/ build/_$(grammar).fbjson
	python main.py i build/_$(grammar).fbjson
	gcc -o build/ci_fuzz-$(grammar) build/_driver.c build/_core.c
	rm build/_*

recursive: main.py build/ build/_$(grammar).fbjson
	python main.py r build/_$(grammar).fbjson
	gcc -o build/cr_fuzz-$(grammar) build/_driver.c build/_core.c
	rm build/_*

clean:
	rm -r build/