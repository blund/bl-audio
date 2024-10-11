
#include "program.h"


void program_setup(program_state*s) {
  s->ctx = cadence_setup(44100);
  s->sine = new_sine();
  s->sine->freq = 100.0f;
}

void program_loop(program_state* s) {
  s->val = gen_sine(s->ctx, s->sine);
}
