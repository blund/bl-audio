#ifndef CADENCE_FFT_H
#define CADENCE_FFT_H

#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#define i16 int16_t
#define i64 int64_t
#define f32 float


typedef struct c32 {
  f32 real;
  f32 imag;
} c32;

// @NOTE - I have marked these as inline, but not actually looked at any assembly

inline c32 c32add(c32 x, c32 y) {
  return (c32){
    .real = x.real + y.real,
    .imag = x.imag + y.imag
  };
}

inline void c32pluseq(c32* x, c32 y) {
  x->real += y.real;
  x->imag += y.imag;
}

inline c32 c32min(c32 x, c32 y) {
  return (c32){
    .real = x.real - y.real,
    .imag = x.imag - y.imag
  };
}

inline c32 c32mul(c32 x, c32 y) {
  return (c32){
    .real = x.real*y.real - x.imag*y.imag,
    .imag = x.real*y.imag + x.imag*y.real
  };
}

inline c32 c32conj(c32 x) {
  return (c32){
    .real = x.real,
    .imag = -x.imag
  };
}

inline c32 c32mul_scalar(c32 x, float scalar) {
  return (c32){
    .real = x.real * scalar,
    .imag = x.imag * scalar
  };
}

inline c32 ftoc(f32 a) {
  return (c32){.real = a, .imag = 0};
}


typedef struct fft_t {
  size_t size;

  int samples_ready;
  
  int  sample_index; // since we receive one sample at a time, we need keep track of when buffers are full
  f32* in_buf;

  f32* overlap_buf; // Used for overlap storage
  c32* buf;
  c32* pers; // persistent fft, is a copy of the fft'd signal. persists when fft is transformed back
} fft_t;



void  new_fft(fft_t* obj, int size);
void  apply_fft(fft_t* obj, float sample);
float apply_ifft(fft_t* obj);

void spectral_shift(fft_t* fft, float factor);

#endif
