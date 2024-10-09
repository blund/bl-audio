/*****************************************************************
 *  synth.h                                                       *
 *  Created on 30.05.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/



#include "context.h"

typedef enum note_flags {
  NOTE_FREE    = 0, // whether note slot is free for a new note
  NOTE_RELEASE = 1, // whether note should start releasing
  NOTE_RESET   = 2, // whether synth should reset internals for this note index
} note_flags;


typedef struct note_t {
  int midi_note;
  float amp;

  int   key;   // used to identify note on/off-events
  int   flags;
} note_t;

void set_flag(note_t* note, note_flags flag);
void unset_flag(note_t* note, note_flags flag);
int  check_flag(note_t* note, note_flags flag);

struct synth_t; // Forward declare 'synth' for osc_t def
typedef float osc_t (cadence_ctx* ctx, struct synth_t* s, int note_index, note_t* note);

#define OSC(name) float name (cadence_ctx* ctx, struct synth_t* s, int note_index, note_t* note)
typedef OSC(osc_t);

typedef struct synth_t {
  osc_t* osc;

  int   poly_count;
  note_t* notes;
} synth_t;

typedef enum note_event {
  NOTE_ON,
  NOTE_OFF,
} note_event;

synth_t* new_synth(int poly_count, osc_t* osc);
void   synth_register_note(synth_t* s, int midi_note, float amp, note_event event);
float  play_synth(cadence_ctx* ctx, synth_t* s);
