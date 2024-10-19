# Cadence
Cadence is a C library for real-time audio processing and synthesis.
It is intended for developing digital instruments, audo plugins and game engines.

## Running
For now the example program only works on linux, depending on [SDL2](https://www.libsdl.org/).

To try the example program, execute:

    make example

This should open a window, where you can select between a polyphonic synthesizer, a sampler, and a grain synthesizer. 
To play, simply use your keyboard!

![An example program built with SDL2, Nuklear, and Cadence](https://www.lundsaunet.no/images/cadence.png)

*What the example program should look like.*

## Usage
Cadence is designed to be embeddable. This means that the core library is not dependent on the platform it is running on. The general design idea is that the user implements platform code for whatever platform they desire, and can use the same Cadence program across any platform. Cadence simply provides utilities, such as oscillators, effects and a polyphony framework, that can be built into any application.

The example program illustrates how to use Cadence. It is structured so that the user program is compiled as a shared library that is loaded by the platform code. 
The platform code is not aware of Cadence or its inner workings; it only knows about the program's exposed interface, consisting of ```program_loop``` and ```midi_event``` functions. These functions define how the platform code can interact with the Cadence program.

It is important to note that Cadence is agnostic to how this interaction is done. Cadence *can* be tightly integrated into an application, at the cost of portability. 

This project is work in progress, and is created for personal explorations in audio programming.

## Running on Teensy
One of the goals of this project is to do sound synthesis on microcontrollers, specifically the [Teensy](https://www.pjrc.com/store/teensy40.html).
An example program is provided, which implements a simple platform layer for the teensy, as well as some example code.

### Building for Teensy
A few things are required to run Cadence on the Teensy:
 - The [Teensy Audio Cape](https://www.pjrc.com/store/teensy3_audio.html)
 - Installing the [Arduino IDE](https://www.arduino.cc/en/software)
 - Installing the [Teensyduino](https://www.pjrc.com/teensy/td_download.html) add-on for Arduino

When these things are installed and sorted, the example program can be compiled and ran by running the following:

    cd teensy
    make
    make run


Here is a video of my Teensy running the example program:

<video width="600" controls>
  <source src="[http://example.com/path/to/your/video.mp4](https://www.lundsaunet.no/video/cadence-teensy.mp4)" type="video/mp4">
  Your browser does not support the video tag.
</video>

*Cadence running the scale from Ocarina of Time on a Teensy with the Audio Cape*

## Further Work
 - Implement more effects (filters, reverb, distortion, etc)
 - Fix fft (currently broken)
 - Refine APIs
 - Lua scripting
 - Platform code for Bela

## Credits
 - The base for the SDL platform audio code is borrowed from [etscrivner](https://github.com/etscrivner/sdl_audio_circular_buffer)
 - Audio files are handled through Sean T. Barret's [stb library](https://github.com/nothings/stb)
 - UI in the example program is made with [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
