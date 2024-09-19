#ifndef CADENCE_MEMORY_H
#define CADENCE_MEMORY_H

#include <stdint.h>

#include "memory.h"

#define PROGRAM_LOOP(name) uint16_t name(memory* memory)
typedef PROGRAM_LOOP(program_loop_t);
//extern "C" PROCESS(process_block);

#define MIDI_EVENT(name) void name(int target, int midi_note, float val, int event_type)
typedef MIDI_EVENT(midi_event_t);

//#define DRAW_IMGUI(name) void name(Memory* memory)
//typedef DRAW_IMGUI(DrawImgui);
//extern "C" DRAW_IMGUI(draw_imgui);

#endif
