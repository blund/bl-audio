# Cadence Audio Engine
Cadence is a work in progress audio engine, built from scratch.

So far, it features a simple oscillator, a sampler, a delay effect, and the capability of generating polyphonic synthesizers using these components.

It is meant for real time processing and sound generation, such as in games and DAW plugins.

## Running
So far, it only works on linux, only depending on alsa.

To try it out for yourself, run:

    make example

This should build the static library, and link it with the example program found in ```examples```

## Usage
Don't! This project is very much a work in progress, and is mostly created for myself. Maybe some release down the line will be usable, but not yet :]

## Further Work
 - Implement more effects (filters, reverb, etc)
 - Lua scripting layer for creating polyphonic synthesizers
 - Some API cleanup...
 - Mac and Windows platform code
 - More general sample handling (add partial caching, and support more than just one channel...)

## Credits
* The base for the SDL platform code is borrowed from [etscrivner](https://github.com/etscrivner/sdl_audio_circular_buffer)
* Audio files are handled through Sean T. Barret's [stb library](https://github.com/nothings/stb)
* UI in the example program is made with [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
