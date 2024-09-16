

.PHONY: example
example:
	make -C src
	make -C example
	./example/sdl-example
