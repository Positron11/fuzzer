build/:
	mkdir build

grammars/%.h:
	python utilities/ggc.py ../grammars/$*.json > $@

iterative: fuzz-iter.c build/ grammars/$(grammar).h
	gcc -o build/gi-fuzzer -include grammars/$(grammar).h fuzz-iter.c

recursive: fuzz-recr.c build/ grammars/$(grammar).h
	gcc -o build/gr-fuzzer -include grammars/$(grammar).h fuzz-recr.c

clean:
	rm -r build/