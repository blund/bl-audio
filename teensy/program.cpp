
extern "C" {
#include "program.h"
}

void program_setup(program_state*s, int sample_rate) {
  s->ctx.sample_rate = sample_rate;
  s->sine.t    = 0.0f;
  s->sine.freq = 440.0f;
}

int16_t program_loop(program_state* s) {
  float sample = gen_sine(&s->ctx, &s->sine);
  
  // Make sure to get this translation logic correct :)
  return (int16_t)(sample*16768.0f)*s->vol;
}
