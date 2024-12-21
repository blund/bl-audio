/*****************************************************************
 *  util.c                                                        *
 *  Created on 18.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/


#include <stdlib.h>
#include "math.h"

#include "cadence.h"
#include "utils.h"

float mtof(int midi) {
  return 440.0f*pow(2, (float)(midi-69.0f) / 12.0f);
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float clamp(float min, float max, float x) {
  return fmax(min, fmin(max, x));
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

// Bezier interpolation between two points with curvature
point bezier(point p1, point p2, float curvature, float t) {
    // Control point based on curvature
    point pc;
    pc.x = (p1.x + p2.x) / 2.0f + curvature * (p2.y - p1.y) / 2.0f;
    pc.y = (p1.y + p2.y) / 2.0f + curvature * (p1.x - p2.x) / 2.0f;

    point result;
    result.x = (1 - t) * (1 - t) * p1.x + 2 * (1 - t) * t * pc.x + t * t * p2.x;
    result.y = (1 - t) * (1 - t) * p1.y + 2 * (1 - t) * t * pc.y + t * t * p2.y;

    return result;

}

float mix(float a, float b, float mix) {
  return (mix/100 * a) + ((100.0f-mix)/100*b);
}
