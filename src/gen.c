/*****************************************************************
 *  gen.c                                                         *
 *  Created on 30.05.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/



#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "gen.h"
#include "context.h"

// -- sine --
sine* new_sine() {
  sine* s = (sine*)malloc(sizeof(sine)); // @NOTE - hardcoded max buffer size
  s->t = 0;
  s->freq = 0;
  return s;
}

float gen_sine(cadence_ctx* ctx, sine* sine) {
  float   wave_period = ctx->sample_rate / sine->freq;
  float   sample      = sin(sine->t);

  sine->t += 2.0f * M_PI * 1.0f / wave_period;

  sine->t = fmod(sine->t, 2.0f*M_PI);
  return sample;
}


// -- phasor --
phasor* new_phasor() {
  phasor* p = (phasor*)malloc(sizeof(phasor)); // @NOTE - hardcoded max buffer size
  p->value = 0;
  p->freq = 0;
  return p;
}

float gen_phasor(cadence_ctx* ctx, phasor* p) {
  double diff =  (double)p->freq / (double)ctx->sample_rate;
  p->value += diff;
  if (p->value > 1.0f) p->value -= 1.0f;
  return p->value;
}

sampler* new_sampler() {
  sampler* sr = malloc(sizeof(sampler));

  // @TODO - error handling
  //int error;
  //stb_vorbis* v = stb_vorbis_open_filename("data/swim7.ogg", &error, &sr->va);
  //stb_vorbis_info vi = stb_vorbis_get_info(v);
  //int samples_per_channel = stb_vorbis_stream_length_in_samples(v);
  //int total_samples = samples_per_channel * vi.channels * 2; // ?????

  //sr->total_samples = samples_per_channel * 1; // * sr->vi.channels;
  //float *output = (float *)malloc(sr->total_samples * sizeof(float));
  //int samples_read = stb_vorbis_get_samples_float(v, 1, &output, total_samples);

  //sr->v = v;
  return sr;
}

int sampler_set_sample(sampler* s, char* sample_path) {
  int error;
  stb_vorbis* v = stb_vorbis_open_filename(sample_path, &error, &s->va);
  stb_vorbis_info vi = stb_vorbis_get_info(v);
  s->v = v;
  return error;
}



// -- sampler --
float play_sampler(sampler* sr) {
  //float sample = output[sample_index + i];
  //sample *= 0.8;
  //write_to_track(2, i, sample);

  float  sample;         // trick to get a single sample
  float* buf = &sample;  // buffer

  int    samples_read = stb_vorbis_get_samples_float(sr->v, 1, &buf, 1);
  if (samples_read == 0) return 0;

  return sample;
}

int register_gen_table(cadence_ctx* ctx, gen_type type) {
  fori(gen_table_size) {
    if (ctx->gt[i].type == GEN_FREE) {
      ctx->gt[i].type = type;
      switch(type) {
      case GEN_SINE: {
	ctx->gt[i].s = new_sine();
      } break;
      case GEN_PHASOR: {
	ctx->gt[i].p = new_phasor();
      } break;
      default: assert(0);
      }
      return i;
    }
  }
  return -1;
}

void gen_table_init(cadence_ctx* ctx) {
  fori(gen_table_size) ctx->gt[i].type = GEN_FREE;
}

void del_gen_table(cadence_ctx* ctx, int i) {
  ctx->gt[i].type = GEN_FREE;
}

void process_gen_table(cadence_ctx* ctx) {
  fori(gen_table_size) {
    gen_type t = ctx->gt[i].type;
    switch(t) {
    case GEN_FREE: break;
    case GEN_SINE: {
      ctx->gt[i].val = gen_sine(ctx, ctx->gt[i].s);
    } break;
    case GEN_PHASOR: {
      ctx->gt[i].val = gen_phasor(ctx, ctx->gt[i].p);
    } break;
    }
  }
}

/*
void write_to_track(cadence_ctx* ctx, int n, int i, float sample) {
  ctx->tracks[n][2*i]   = sample;
  ctx->tracks[n][2*i+1] = sample;
}
*/

/*
void mix_tracks(cadence_ctx* ctx) {
  fori(ctx->a->info.frames) {
    float sample_l = 0;
    float sample_r = 0;

    for (int n = 0; n < num_tracks; n++) {
      sample_l += ctx->a->tracks[n][2*i];
      sample_r += ctx->a->tracks[n][2*i+1];
    }

    // sample to 16 bit int and copy to alsa buffer
    ctx->a->info.buffer[2*i]   = (int16_t)(32768.0f * sample_l);
    ctx->a->info.buffer[2*i+1] = (int16_t)(32768.0f * sample_r);
  }
}
*/
