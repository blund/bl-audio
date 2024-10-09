/*****************************************************************
 *  gen.h                                                         *
 *  Created on 30.05.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/



#ifndef GEN_H
#define GEN_H

#include <stdint.h>

#define STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>

typedef struct cadence_ctx cadence_ctx;

typedef struct sine {
  double t;
  float freq;
} sine;
sine* new_sine();
float gen_sine(struct cadence_ctx* ctx, sine* sine);

typedef struct phasor {
  double value;
  float freq;
} phasor;
phasor* new_phasor();
float gen_phasor(struct cadence_ctx* ctx, phasor* p);

typedef struct sampler {
  stb_vorbis* v;
  stb_vorbis_alloc va;
  stb_vorbis_info vi;
} sampler;

int sampler_set_sample(sampler* s, char* sample_path);
float play_sampler(sampler* sr);

// Set sample index for sampler
void sampler_seek(sampler* s, int sample_index);
// Get length of currently loaded sample
int sampler_length(sampler* s); 

// -- gen table --
/*
  The gen table is meant for generators you want to use in
  several instruments per fram iteration, like modulators in
  polyphonic synths.
  The generators in the table will be updated at the beginning
  of each frame processed, and its value will remain the same
  for every other generator processed.
  Specifically, instruments can register new generators for the
  tables, and will have access to them through their registered
  id.
 */
typedef enum gen_type {
  GEN_FREE,
  GEN_SINE,
  GEN_PHASOR,
} gen_type;

struct gen_table {
  float val;
  union {
    sine* s;
    phasor* p;
  };
  gen_type type;
  int free;
};
void gen_table_init(struct cadence_ctx* ctx);
int  register_gen_table(struct cadence_ctx* ctx, gen_type type);
void del_gen_table(struct cadence_ctx* ctx, int i);
void process_gen_table(struct cadence_ctx* ctx);

#endif
