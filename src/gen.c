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

float gen_sine(cae_ctx* ctx, sine* sine) {
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

float gen_phasor(cae_ctx* ctx, phasor* p) {
  double diff =  (double)p->freq / (double)ctx->sample_rate;
  p->value += diff;
  if (p->value > 1.0f) p->value -= 1.0f;
  return p->value;
}

// -- delay --
delay* new_delay(cae_ctx* ctx) {
  delay* d = (delay*)malloc(sizeof(delay)); // @NOTE - hardcoded max buffer size
  d->buf_size = 10*ctx->sample_rate;
  d->buffer = malloc(d->buf_size * sizeof(float));

  // clear out buffer @NOTE - might be unecessary
  for (int i = 0; i < d->buf_size; i++) {
    d->buffer[i] = 0;
  }
  return d;
}

sampler* new_sampler() {
  sampler* sr = malloc(sizeof(sampler));

  int error;
  stb_vorbis* v = stb_vorbis_open_filename("data/swim7.ogg", &error, &sr->va);
  stb_vorbis_info vi = stb_vorbis_get_info(v);
  //int samples_per_channel = stb_vorbis_stream_length_in_samples(v);
  //int total_samples = samples_per_channel * vi.channels * 2; // ?????

  //sr->total_samples = samples_per_channel * 1; // * sr->vi.channels;
  //float *output = (float *)malloc(sr->total_samples * sizeof(float));
  //int samples_read = stb_vorbis_get_samples_float(v, 1, &output, total_samples);

  sr->v = v;
  return sr;
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

float apply_delay(cae_ctx* ctx, delay* d, float sample, float delay_ms, float feedback) {
  d->read_offset = delay_ms * ctx->sample_rate; // @NOTE - hardcoded samplerate
  // read from delay buffer
  uint32_t index = (d->write_head - d->read_offset) % d->buf_size;
  float delayed  = d->buffer[index];

  // write back + feedback!
  d->buffer[d->write_head % d->buf_size] = sample + delayed * feedback;

  // increment delay
  d->write_head++;
  return delayed;
}

int register_gen_table(cae_ctx* ctx, gen_type type) {
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

void gen_table_init(cae_ctx* ctx) {
  fori(gen_table_size) ctx->gt[i].type = GEN_FREE;
}

void del_gen_table(cae_ctx* ctx, int i) {
  ctx->gt[i].type = GEN_FREE;
}

void process_gen_table(cae_ctx* ctx) {
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
void write_to_track(cae_ctx* ctx, int n, int i, float sample) {
  ctx->tracks[n][2*i]   = sample;
  ctx->tracks[n][2*i+1] = sample;
}
*/

/*
void mix_tracks(cae_ctx* ctx) {
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
