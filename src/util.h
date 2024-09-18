#ifndef CADENCE_UTIL_H
#define CADENCE_UTIL_H

#include "context.h"
float mtof(int midi);

typedef struct line_t {
  float len_samples;
  float rem_samples;
  float start_val;
  float end_val;
} line_t;

void set_line(cadence_ctx* ctx, line_t* l, float len_secs, float start, float end);
void reset_line(line_t* l);
float line(line_t* l, int* done);


typedef struct adsr_t {
  line_t atk;
  line_t rel;
} adsr_t;

void reset_adsr(adsr_t* l);
float adsr(adsr_t* adsr, int trig_rel, int* done);

#endif
