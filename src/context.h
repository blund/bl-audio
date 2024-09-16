#ifndef CONTEXT_H
#define CONTEXT_H

#include <alsa/asoundlib.h>

#include "gen.h"

// Helper for for loops :)
#define fori(lim) for(int i = 0; i < lim; i++)

#define num_tracks 4
#define track_size 256
#define channels_pr_track 2

typedef struct gen_table gen_table;

typedef struct audio_info {
  int rc;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int sample_rate;
  int dir;
  snd_pcm_uframes_t frames;
  int16_t *buffer;
  uint32_t buffer_size;
} audio_info;

typedef struct audio {
  audio_info info;
  float tracks[num_tracks][channels_pr_track*track_size];
} audio;

#define gen_table_size 64
typedef struct cae_ctx {
  audio* a;
  struct gen_table gt[gen_table_size];
} cae_ctx;

void write_to_track(cae_ctx* ctx, int n, int i, float sample);
void mix_tracks(cae_ctx* ctx);
void write_to_track(cae_ctx* ctx, int n, int i, float sample);

#endif
