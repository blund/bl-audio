#ifndef GEN_H
#define GEN_H

#include <stdint.h>

#define STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>

typedef struct cae_ctx cat_ctx;

typedef struct sine {
  double t;
  float freq;
} sine;
sine* new_sine();
float gen_sine(struct cae_ctx* ctx, sine* sine);

typedef struct phasor {
  double value;
  float freq;
} phasor;
phasor* new_phasor();
float gen_phasor(struct cae_ctx* ctx, phasor* p);

typedef struct sampler {
  stb_vorbis* v;
  stb_vorbis_alloc va;
  stb_vorbis_info vi;
  int total_samples;
  int sample_index;
} sampler;
sampler* new_sampler();
float play_sampler(sampler* sr);

typedef struct delay {
  float*   buffer;
  uint32_t buf_size;
  uint32_t write_head;
  uint32_t read_offset;
} delay;
delay* new_delay(struct cae_ctx* ctx);
float apply_delay(struct cae_ctx* ctx, delay* d, float sample, float delay_ms, float feedback);

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
int register_gen_table(struct cae_ctx* ctx, gen_type type);
void del_gen_table(struct cae_ctx* ctx, int i);
void process_gen_table(struct cae_ctx* ctx);

#endif
