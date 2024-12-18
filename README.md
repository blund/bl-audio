# Cadence
A library for real-time audio processing and synthesis.


## What is this project?
Cadence is a small C library that provides the building blocks for audio processing and synthesis. It is intended as a foundation for building digital instruments, audio plugins and audio for game engines. The library is written in portable C code, so that your program can be compiled for any platform, given that you also define the platform interface for it to interact with. So far, the only platform layers are for [SDL2](https://www.libsdl.org/) and the [Teensy](https://www.pjrc.com/store/teensy40.html) microcontroller. 

Cadence currently provides some basic oscillators, effects, midi-support, and a polyphony framework. More utilities will be implemented as I need them :)

The primary motivation for creating Cadence is exploratory programming and having great fun along the way. It is as a lightweight "alternative" to something like [JUCE](https://juce.com/). The goal for Cadence is to have a library that can easily embedded into any project that requires sound.

Please note that this project is very much a work in progress, and is far from mature. Everything is subject to change.

### What currently works?

- Oscillators (sine, phasor)
- Basic effects (reverb, delay, low-pass fitler)
- Spectral processing (pitch shifter)
- Interactive GUI example on Linux (powered by [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear))
- Teensy platform layer (see video below!)

## Running
### Running the Linux example
#### Dependencies
 - [SDL2](https://www.libsdl.org/) installed (for audio and video)
 - ALSA libraries (for midi)

Note that the example program only works on Linux for now.

To run the example program, execute the following in the base folder to compile and execute it:

    make example

This should open a window, where you can select between a polyphonic synthesizer, a sampler, and a granular synthesizer. 
To play, simply use your keyboard!

![An example program built with SDL2, Nuklear, and Cadence](https://github.com/user-attachments/assets/36b352b8-e910-4b14-a2e5-ab5f9f8670c5)

*The example program. Built with SDL2, Nuklear and Cadence.*


### Running the Teensy example
One of the goals of this project is to do sound synthesis on microcontrollers, specifically the [Teensy](https://www.pjrc.com/store/teensy40.html).
An example program is provided, which implements a simple platform layer for the teensy, as well as some example code.

#### Building for Teensy
A few things are required to run Cadence on the Teensy:
 - The [Teensy 4.0 Development Board](https://www.pjrc.com/store/teensy40.html)
 - The [Teensy Audio Adaptor](https://www.pjrc.com/store/teensy3_audio.html)
 - Installing the [Arduino IDE](https://www.arduino.cc/en/software)
 - Installing the [Teensyduino](https://www.pjrc.com/teensy/td_download.html) add-on for Arduino

When these things are installed and sorted, the example program can be compiled and ran by running the following:

    cd teensy
    make
    make run


Here is a video of my Teensy running the example program:


https://github.com/user-attachments/assets/8af8bb0a-924c-4b65-9270-89031b691841

*The example program for the Teensy, featuring the scale from Ocaria of Time.*

## Further Work
 - Implement more effects (filters, distortion, etc)
 - More Teensy support
   - Effect compatibility, reverb, delay
   - FFT compatibility
 - Refine APIs
 - Lua scripting
 - Platform code for Bela
 - Fixing code hot reloading
 - Mac support
 - Windows support

## Credits
 - The base for the SDL platform audio code is borrowed from [etscrivner](https://github.com/etscrivner/sdl_audio_circular_buffer)
 - Audio files are handled through Sean T. Barret's [stb library](https://github.com/nothings/stb)
 - UI in the example program is made with [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
