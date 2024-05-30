
.PHONY: example
example:
	make -C ..
	gcc example.c ../cadence.a -I.. -I../include -lm -lasound -o example

.PHONY: run
run: example
	./example
