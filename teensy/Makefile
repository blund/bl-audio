
compile:
	@rm -r src
	@mkdir src

	@cp ../src/*.c ./src
	@cp ../src/*.h ./src
	@cp -r ../src/include ./src

	@rm src/fft.h   # fft breaks everything so we remove it for now :)
	@rm src/fft.c   #
	@touch src/fft.h # needed for the hack!

	arduino-cli compile --fqbn teensy:avr:teensy40 . --output-dir ./build

run:
	teensy_loader_cli --mcu=TEENSY40 -w ./build/teensy.ino.hex
