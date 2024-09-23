

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

#define i16 int16_t
#define i64 int64_t
#define f32 float

#define fori(end) for (int i = 0; i < end; i++)

const float pi2 = 6.28318548f;

typedef enum fft_stages {
  FIRST_ITERATION,
  FIRST_ITERATION_DONE,
} fft_stages;

typedef struct fft_t {
  size_t size;

  fft_stages stage;
  int samples_ready;
  
  int  sample_count; // since we receive one sample at a time, we need keep track of when buffers are full
  f32* in_buf;

  f32* overlap_buf; // Used for overlap storage
  f32* real;
  f32* imag;
} fft_t;



void new_fft(fft_t* obj, int size) {
  obj->size  = size;
  obj->real = malloc(obj->size*sizeof(f32));
  obj->imag = malloc(obj->size*sizeof(f32));
  obj->overlap_buf  = malloc(obj->size/2*sizeof(f32));
  obj->in_buf = malloc(obj->size/2*sizeof(f32));

  obj->stage = FIRST_ITERATION;
  obj->samples_ready = 0;

  // @NOTE - probabl unecessary
  fori(obj->size) obj->real[i] = 0;
  fori(obj->size) obj->imag[i] = 0;
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
      fori(obj->size/2) obj->real[i] = obj->overlap_buf[i];
      fori(obj->size/2) obj->overlap_buf[i] = obj->real[i+obj->size/2] = obj->in_buf[i];

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
  return obj->real[obj->sample_count];
}

void fft(fft_t signal) {
  f32* real = signal.real;
  f32* imag = signal.imag;

  //assert(signal.size < INT_MAX);
  int n = (int)signal.size;
  for (int i = 1, j = 0; i < n; i++) {
    int bit = n >> 1;
    for (; j & bit; bit >>= 1)
      j ^= bit;
    j ^= bit;

    if (i < j) {
      float tmp = real[i];
      real[i] = real[j];
      real[j] = tmp;
      tmp = imag[i];
      imag[i] = imag[j];
      imag[j] = tmp;
    }
  }

  for (int len = 2; len <= n; len <<= 1) {
    float ang = pi2 / (float)len;
    float wlen_real = cosf(ang);
    float wlen_imag = sinf(ang);
    for (int i = 0; i < n; i += len) {
      float w_real = 1;
      float w_imag = 0;
      for (int j = 0; j < len / 2; j++) {
	float u_real = real[i+j];
	float u_imag = imag[i+j];
	//mult();
	float v_real = real[i+j+len/2] * w_real - imag[i+j+len/2] * w_imag;
	float v_imag = real[i+j+len/2] * w_imag + imag[i+j+len/2] * w_real;
	real[i+j] = u_real + v_real;
	imag[i+j] = u_imag + v_imag;
	real[i+j+len/2] = u_real - v_real;
	imag[i+j+len/2] = u_imag - v_imag;
	//mult();
	float w_real_next = w_real * wlen_real - w_imag * wlen_imag;
	//mult();
	w_imag = w_real * wlen_imag + w_imag * wlen_real;
	w_real = w_real_next;
      }
    }
  }
}

void ifft(fft_t signal) {
  // conjugate the complex numbers
  for (int i = 0; i < signal.size; i++)
    signal.imag[i] = -signal.imag[i];

  // apply the FFT
  fft(signal);

  // conjugate the complex numbers again and divide by N
  for (int i = 0; i < signal.size; i++) {
    signal.imag[i] = -signal.imag[i];
    signal.real[i] /= (float)signal.size;
    signal.imag[i] /= (float)signal.size;
  }
}

void fft_mul_arrs(fft_t sig, fft_t ir) {
  f32 tmp_real;
  f32 tmp_imag;

  fori(sig.size) {
    //mult();
    //mult();
    tmp_real = sig.real[i]*ir.real[i] - sig.imag[i]*ir.imag[i];
    tmp_imag = sig.real[i]*ir.imag[i] + sig.imag[i]*ir.real[i];

    sig.real[i] = tmp_real;
    sig.imag[i] = tmp_imag;
  }
}

void hann(fft_t sig, int block_size) {
  for (int i = 0; i < block_size; i++) {
    const float a0 = 25.0f/46.0f;
    float multiplier = 0.5f*(1.0f - cosf(pi2*(float)i/2047.0f));
    sig.real[i] *= multiplier;
  }
}
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
