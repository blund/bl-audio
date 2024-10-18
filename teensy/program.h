
extern "C" {
#include "src/cadence.h"
#include "stdint.h"
}

typedef struct program_state {
  cadence_ctx*  ctx;
  float vol;
  uint8_t* mem;
  uint32_t mem_size;
} program_state;

void program_setup(program_state*s, int sample_rate);
int16_t program_loop(program_state* state);
void midi_event(int midi_note, note_event event_type);
