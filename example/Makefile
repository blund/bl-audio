
.PHONY: example
example:
	make -C ..
	gcc example.c ../cadence.a -I.. -I../include -lm -lasound -o example

.PHONY: run
run: example
	./example

piano:
	make -C ..
#gcc piano.c ../cadence.a -Iinclude -I../include -lglfw -lGL -lm -lGLU -lGLEW -lasound -lpthread

.PHONY: term
term:
	gcc terminal.c ../cadence.a -I.. -I../include -lm -lasound -o term && ./term


.PHONY: sdl
sdl:
	gcc sdl.c ../cadence.a -I.. -I../include -lm `pkg-config --cflags --libs sdl2` -o sdl && ./sdl
