/*****************************************************************
 *  util.c                                                        *
 *  Created on 18.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/



#include "math.h"
#include "malloc.h"

#include "cadence.h"

float mtof(int midi) {
  return 440.0f*pow(2, (float)(midi-69.0f) / 12.0f);
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

int rand_int(int min, int max) {
  float scale = rand()/(float)RAND_MAX; /* [0, 1.0] */
  return (int)min+scale*(max-min);      /* [min, max] */
}

float rand_float(float min, float max) {
  float scale = rand()/(float)RAND_MAX; /* [0, 1.0] */
  return min+scale*(max-min);           /* [min, max] */
}

void set_line(cadence_ctx* ctx, line_t* l, float len_secs, float start, float end) {
  l->len_samples = len_secs*ctx->sample_rate;
  l->rem_samples = len_secs*ctx->sample_rate;
  l->start_val = start;
  l->end_val   = end;
}

void reset_line(line_t* l) {
  l->rem_samples = l->len_samples;
}

float line(line_t* l, int* done) {
  if (done) *done = 0;

  float a = l->start_val;
  float b = l->end_val;
  float x = l->len_samples;
  float y = l->rem_samples;

  if (l->rem_samples > 0) {
    l->rem_samples -= 1;
  } else if (done) {
    *done = 1;
  }
  
  return b + (a - b) * y/x;
}

void reset_adsr(adsr_t* adsr) {
  reset_line(&adsr->atk);
  reset_line(&adsr->rel);
}

// attack and release
float adsr(adsr_t* adsr, int trig_rel, int* done) {
  float val = line(&adsr->atk, NULL);

  if (trig_rel) {
    val *= line(&adsr->rel, done);
  }

  return val;
}


