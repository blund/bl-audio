# Cadence Audio Engine
Cadence is an embeddable library for real-time audio processing and synthesis.
It is intended to be used for developing digital instruments, DAW plugins and audio for game engines.

## Running
So far, it only works on linux, depending on [SDL2](https://www.libsdl.org/).

To try the example program, run:

    make example

This should open a window running a basic polyphonic synthesizer. To play, use your computer keyboard!

## Usage
Cadence is designed to be embeddable. This means that the core library is not dependent on the platform it is running on. The general design idea is that the user implements platform code for whatever platform they desire, and can use the same Cadence program across any platform. Cadence simply provides utilities, such as oscillators, effects and a polyphony framework, that can be built into any application.

The example program illustrates how to use Cadence. It is structured so that the user program is compiled as a shared library that is loaded by the platform code. 
The platform code is not aware of Cadence or its inner workings; it only knows about the program's exposed interface, consisting of ```program_loop``` and ```midi_event``` functions. These functions define how the platform code can interact with the Cadence program.

It is important to note that Cadence is agnostic to how this interaction is done. Cadence *can* be tightly integrated into an application, at the cost of portability. 

This project is work in progress, and is created for personal explorations in audio programming.

## Further Work
 - Implement more effects (filters, reverb, etc)
 - Lua scripting layer
 - Some API cleanup...
 - Mac and Windows platform code

## Credits
 - The base for the SDL platform code is borrowed from [etscrivner](https://github.com/etscrivner/sdl_audio_circular_buffer)
 - Audio files are handled through Sean T. Barret's [stb library](https://github.com/nothings/stb)
 - UI in the example program is made with [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
