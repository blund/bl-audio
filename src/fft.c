

/*

  @NOTE!!!!!!!

  When implementing functions that operate on frequency spectra it is important
  that the function implementation is prefaces by a check as to whether the
  samples are ready for processing.

  The fft's samples will only be ready for processing when the in_buffer has been
  filled and the apply_fft function has done its operation.
  All subsequent spectral effects must be applied after this, before apply_ifft.

  In order to function properly, fft functions should be used as such:

  apply_fft(obj);
  fft_effect1(obj);
  fft_effect2(obj);
  float sample = apply_ifft(obj);
  

  Here is an example implementation of a spectral effect that multiplies the
  amplitude of each frequency by 0.5;

void test_process(fft_t* obj) {
  if (!obj->samples_ready) return;
  float sr   = 44100;
  float fft_size = 1024;

  float hz_per_bin = sr/fft_size;

  fori(1024) { obj->real[i] *= 0.5; obj->imag[i] *= 0.5; }
}



*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <complex.h>

#include "fft.h"

#define i16 int16_t
#define i64 int64_t
#define f32 float

#define fori(end) for (int i = 0; i < end; i++)

const float pi2 = 6.28318548f;


void new_fft(fft_t* obj, int size) {
  obj->size  = size;
  obj->overlap_buf  = malloc(obj->size/2*sizeof(f32));
  obj->in_buf = malloc(obj->size/2*sizeof(f32));
  obj->buf = malloc(obj->size*sizeof(f32 complex));

  obj->stage = FIRST_ITERATION;
  obj->samples_ready = 0;

  // @NOTE - probabl unecessary
  fori(obj->size) obj->buf[i] = 0;
}

void fft(fft_t signal);
void ifft(fft_t signal);

void apply_fft(fft_t* obj, float sample) {
  if (obj->sample_count < obj->size/2) {
    obj->in_buf[obj->sample_count] = sample;
    obj->sample_count++;
  }

  if (obj->sample_count == obj->size/2) {
    //puts("trigger");
    obj->sample_count = 0;
    if (obj->stage == FIRST_ITERATION) {
      // On first full buffer, just fill the overlap buf
      fori(obj->size/2) obj->overlap_buf[i] = obj->in_buf[i];
      obj->stage = FIRST_ITERATION_DONE; // Used for "apply_ifft"
    } else {
      obj->samples_ready = 1;
      // On second full buffer onwards, fill the first and second halves of the real buf as well
      fori(obj->size/2) obj->buf[i] = obj->overlap_buf[i];
      fori(obj->size/2) obj->overlap_buf[i] = obj->buf[i+obj->size/2] = obj->in_buf[i];

      //puts("2:"); fori(obj->size) printf("%-2.0f, ", obj->real[i]); puts("");

      fft(*obj);
    }
  }
}

float apply_ifft(fft_t* obj) {
  // Still buffering the first samples before first full window is filled
  if (obj->stage == FIRST_ITERATION) return 0;

  // If we have a new set of samples ready, convert these
  if (obj->samples_ready) {
    obj->samples_ready = 0;
    ifft(*obj);
  }

  // return the equivalent of the sample we are writing
  return obj->buf[obj->sample_count];
}

void multiply_bin(fft_t* obj, int i, float val) {
    int i_sym = obj->size-i;
    obj->buf[i]     *= val;
    obj->buf[i_sym] *= val;
}

void fft(fft_t signal) {
  float complex* data = signal.buf;

  //assert(signal.size < INT_MAX);
  int n = (int)signal.size;
  
  // Bit-reversal permutation
  for (int i = 1, j = 0; i < n; i++) {
    int bit = n >> 1;
    for (; j & bit; bit >>= 1)
      j ^= bit;
    j ^= bit;

    if (i < j) {
      float complex tmp = data[i];
      data[i] = data[j];
      data[j] = tmp;
    }
  }

  // FFT calculation
  for (int len = 2; len <= n; len <<= 1) {
    float ang = pi2 / (float)len;
    float complex wlen = cosf(ang) + sinf(ang) * I;  // wlen = e^(-i * 2 * pi / len)

    for (int i = 0; i < n; i += len) {
      float complex w = 1.0f + 0.0f * I;
      
      for (int j = 0; j < len / 2; j++) {
        float complex u = data[i + j];
        float complex v = data[i + j + len / 2] * w;

        data[i + j] = u + v;
        data[i + j + len / 2] = u - v;

        w *= wlen;  // Update w for next element
      }
    }
  }
}

void ifft(fft_t signal) {
  // Step 1: Conjugate the complex numbers
  for (int i = 0; i < signal.size; i++) {
    signal.buf[i] = conjf(signal.buf[i]);
  }

  // Step 2: Apply the FFT
  fft(signal);

  // Step 3: Conjugate the complex numbers again and divide by the size
  for (int i = 0; i < signal.size; i++) {
    signal.buf[i] = conjf(signal.buf[i]) / (float)signal.size;
  }
}

void fft_mul_arrs(fft_t sig, fft_t ir) {
  f32 tmp_real;
  f32 tmp_imag;

  fori(sig.size) {
    //mult();
    //mult();
    sig.buf[i] *= ir.buf[i];
  }
}

void hann(fft_t sig, int block_size) {
  for (int i = 0; i < block_size; i++) {
    const float a0 = 25.0f/46.0f;
    float multiplier = 0.5f*(1.0f - cosf(pi2*(float)i/2047.0f));
    sig.buf[i] *= multiplier;
  }
}

/*
int main() {
  

  // Manage full signal
  int offset = 0;
  float full_signal[4096];
  fori(4096) full_signal[i] = i;

  fft_t obj;
  new_fft(&obj, 32);

  fori(0) {
    apply_fft(&obj, full_signal[i]);

    float sample = apply_ifft(&obj);
    printf("%-2.0f, ", sample);
  }
  puts("");

  return 0;

  // Første iterasjon: vent på neste sett med samples:
  fori(obj.size/2) obj.overlap_buf[i] = full_signal[offset + i];

  puts("1:"); fori(obj.size) printf("%-2.0f, ", obj.real[i]); puts("");

  // Andre iterasjon: fyll inn resterende samples, ta vare på de
  offset += obj.size/2;
  fori(obj.size/2) obj.real[i] = obj.overlap_buf[i];
  fori(obj.size/2) obj.overlap_buf[i] = obj.real[i+obj.size/2] = full_signal[offset+i];
  fft(obj); ifft(obj);

  puts("2:"); fori(obj.size) printf("%-2.0f, ", obj.real[i]); puts("");

  // Tredje iterasjon: kopier forrige samples fra overlap_buffer, lagre nye
  offset += obj.size/2;
  fori(obj.size/2) obj.real[i] = obj.overlap_buf[i];
  fori(obj.size/2) obj.overlap_buf[i] = obj.real[i+obj.size/2] = full_signal[offset+i];
  fft(obj); ifft(obj);
  
  puts("3:"); fori(obj.size) printf("%-2.0f, ", obj.real[i]); puts("");

  // Tredje iterasjon: kopier forrige samples fra overlap_buffer, lagre nye
  offset += obj.size/2;
  fori(obj.size/2) obj.real[i] = obj.overlap_buf[i];
  fori(obj.size/2) obj.overlap_buf[i] = obj.real[i+obj.size/2] = full_signal[offset+i];
  fft(obj); ifft(obj);
  
  puts("4:"); fori(obj.size) printf("%-2.0f, ", obj.real[i]); puts("");


  fft(obj);
  ifft(obj);

  //fori(obj.size) printf("(%.1f), ", obj.real[i]);
  puts("");
}
*/
