

.PHONY: example
example:
	make -C src
	make -C example lib
	make -C example sdl
	./example/sdl
