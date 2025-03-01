/*****************************************************************
 *  context.h                                                     *
 *  Created on 18.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/



#ifndef CONTEXT_H
#define CONTEXT_H

#include "gen.h"

// #define num_tracks 4
// #define track_size 256
// #define channels_pr_track 2

typedef struct gen_table gen_table;

#define gen_table_size 64
typedef struct cadence_ctx {
  int initialized;
  //float tracks[num_tracks][channels_pr_track*track_size];
  int sample_rate;
  struct gen_table gt[gen_table_size];

  void* (*alloc)(uint64_t size);
  void* (*free)(uint64_t size);

} cadence_ctx;

// @NOTE - not sure what to do with these
// void write_to_track(cae_ctx* ctx, int n, int i, float sample);
// void mix_tracks(cae_ctx* ctx);
// void write_to_track(cae_ctx* ctx, int n, int i, float sample);

cadence_ctx* cadence_setup(int sample_rate);

#endif
