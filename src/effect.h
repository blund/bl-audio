#ifndef CADENCE_EFFECT_H
#define CADENCE_EFFECT_H

#include "context.h"

typedef struct butlp {
  float cutoff_freq;
  float a0, a1, a2;
  float b0, b1, b2;
  float x1, x2;  // Past input samples
  float y1, y2;  // Past output samples
} butlp;

butlp* new_butlp(cadence_ctx* ctx, float freq);
float apply_butlp(cadence_ctx* ctx, butlp *filter, float input, float cutoff_freq);

typedef struct delay {
  float*   buffer;
  uint32_t buf_size;
  uint32_t write_head;
  uint32_t read_offset;
} delay;
delay* new_delay(struct cadence_ctx* ctx);
float apply_delay(struct cadence_ctx* ctx, delay* d, float sample, float delay_ms, float feedback);

#endif
