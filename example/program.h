/*****************************************************************
 *  program.h                                                     *
 *  Created on 19.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/

#ifndef CADENCE_MEMORY_H
#define CADENCE_MEMORY_H

#include <stdint.h>

#include "nuklear/nuklear.h"

#include "memory.h"


#define PROGRAM_LOOP(name) uint16_t name(memory* memory)
typedef PROGRAM_LOOP(program_loop_t);


#define DRAW_GUI(name) void name(struct nk_context* ctx)
typedef DRAW_GUI(draw_gui_t);

#define MIDI_EVENT(name) void name(int target, int midi_note, float val, int event_type)
typedef MIDI_EVENT(midi_event_t);

#endif
