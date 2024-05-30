
.PHONY: cadence
cadence:
	gcc cadence.c gen.c synth.c alsa_audio.c include/stb/stb_vorbis.c -g -Iinclude -lasound -lm -o cadence && ./cadence
