
cadence.a: *.c
	gcc -O2 gen.c synth.c alsa_audio.c include/stb/stb_vorbis.c -Iinclude -lasound -lm -c
	ar rcs cadence.a gen.o synth.o alsa_audio.o stb_vorbis.o
	rm gen.o synth.o alsa_audio.o stb_vorbis.o

.PHONY: example
example:
	make -C example
	cd example && ./example
