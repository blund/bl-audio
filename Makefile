
.PHONY: cadence
cadence:
	gcc cadence.c synth.c include/stb/stb_vorbis.c -g -Iinclude -lasound -lm -o cadence && ./cadence
