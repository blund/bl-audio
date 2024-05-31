
.PHONY: example
example:
	make -C ..
	gcc example.c ../cadence.a -I.. -I../include -lm -lasound -o example

.PHONY: run
run: example
	./example

piano:
	make -C ..
	gcc piano.c ../cadence.a -Iinclude -I../include -lglfw -lGL -lm -lGLU -lGLEW -lasound -lpthread
