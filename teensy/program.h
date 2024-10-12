
#include "src/cadence.h"

typedef struct program_state {
  cadence_ctx  ctx;
  sine_t       sine;

  float val;
} program_state;

void program_setup(program_state*s, int sample_rate);
int16_t program_loop(program_state* state);
