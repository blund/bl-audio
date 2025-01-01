

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
#include <string.h>

#include "fft.h"
#include "util.h"


const float pi2 = 6.28318548f;

void bit_reversal(fft_t signal);

void new_fft(fft_t* obj, int size) {
  obj->size  = size;

  // These are used for storing the input before processing
  obj->overlap_buf  = malloc(obj->size*sizeof(f32));
  obj->in_buf       = malloc(obj->size*sizeof(f32));

  // This is where the fft is stored after processed
  obj->buf          = malloc(obj->size*sizeof(c32));
  obj->pers         = malloc(obj->size*sizeof(c32));

  obj->samples_ready = 0;
  obj->sample_index = 0;

  // @NOTE - probabl unecessary
  fori(obj->size) obj->buf[i] = (c32){0,0};
}

void fft(fft_t signal);
void ifft(fft_t signal);

void apply_fft(fft_t* obj, float sample) {
  // We are still filling up the buffer for processing
  if (obj->sample_index < obj->size) {
    obj->in_buf[obj->sample_index] = sample;
    obj->sample_index++;
  }

  // The buffer is full. We can perform fft
  if (obj->sample_index == obj->size) {

    // Notify that apply_ifft can spit out samples;
    obj->samples_ready = 1;

    // Reset sample index for playback etc
    obj->sample_index = 0;

    fori(obj->size) {
      // Store input buffer to processing buffer
      obj->buf[i] = ftoc(obj->in_buf[i]);

      // Zero pad second half of processing buffer
      obj->buf[obj->size+i] = ftoc(0);
    }

    // Do the magic
    bit_reversal(*obj);
    fft(*obj);
  }
}

float apply_ifft(fft_t* obj) {
  if (obj->samples_ready) {
    obj->samples_ready = 0;

    // Undo the magic
    bit_reversal(*obj);
    ifft(*obj);

    fori(obj->size) {
      // Add the old overlap to the first half of the buffer
      obj->buf[i] = c32add(obj->buf[i], ftoc(obj->overlap_buf[i]));

      // Save the new overlap from the second half
      obj->overlap_buf[i] = obj->buf[obj->size+i].real;
    }
  }

  return obj->buf[obj->sample_index].real;
}

/*
void multiply_bin(fft_t* obj, int i, float val) {
    int i_sym = obj->size-i;
    obj->buf[i]     *= val;
    obj->buf[i_sym] *= val;
}
*/


void bit_reversal(fft_t signal) {
  c32* data = signal.buf;
  int n = (int)signal.size;

  // Bit-reversal permutation
  for (int i = 1, j = 0; i < n; i++) {
    int bit = n >> 1;
    for (; j & bit; bit >>= 1)
      j ^= bit;
    j ^= bit;

    if (i < j) {
      c32 tmp = data[i];
      data[i] = data[j];
      data[j] = tmp;
    }
  }
}

void fft(fft_t signal) {
  c32* data = signal.buf;

  //assert(signal.size < INT_MAX);
  int n = (int)signal.size;
  // FFT calculation
  for (int len = 2; len <= n; len <<= 1) {
    float ang = pi2 / (float)len;
    c32 wlen = (c32){.real = cosf(ang),
		     .imag = sinf(ang)};  // wlen = e^(-i * 2 * pi / len)

    for (int i = 0; i < n; i += len) {
      c32 w = ftoc(1.0);
      
      for (int j = 0; j < len / 2; j++) {
        c32 u = data[i + j];
        c32 v = c32mul(data[i + j + len / 2], w);

        data[i + j] = c32add(u, v);
        data[i + j + len / 2] = c32min(u, v);

        w = c32mul(w, wlen);  // Update w for next element
      }
    }
  }
}

void ifft(fft_t signal) {

  // Step 1: Conjugate the complex numbers
  for (int i = 0; i < signal.size; i++) {
    signal.buf[i] = c32conj(signal.buf[i]);
  }

  // Step 2: Apply the FFT
  fft(signal);

  // Step 3: Conjugate the complex numbers again and divide by the size
  for (int i = 0; i < signal.size; i++) {
    signal.buf[i] = c32mul_scalar(c32conj(signal.buf[i]), 1.0f/signal.size);
  }
}

/*
void fft_mul_arrs(fft_t sig, fft_t ir) {
  f32 tmp_real;
  f32 tmp_imag;

  fori(sig.size) {
    //mult();
    //mult();
    sig.buf[i] *= ir.buf[i];
  }
}
*/

/*
float window[1024*32] = {-69};

void generate_hann_window(float* window, int size) {
    for (int n = 0; n < size; n++) {
        window[n] = 0.5f * (1.0f - cosf((2.0f * M_PI * n) / (size - 1)));
    }
}

void apply_hann_window(float complex* signal, int size) {
  if (window[0] == -69) generate_hann_window(window, size);
    for (int i = 0; i < size; i++) {
        signal[i] *= window[i]; // Element-wise multiplication
    }
}
*/

/* Effects */

// Function to perform spectral shifting by cents
void spectral_shift(fft_t* fft, float shift_factor) {
    c32 temp[fft->size]; // Temporary buffer for the shifted spectrum
    memset(temp, 0, sizeof(temp)); // Clear the buffer

    int half_size = fft->size / 2;

    // Process positive frequencies [0, N/2]
    for (int i = 0; i <= half_size; i++) {
        float original_bin = (float)i; // Original bin index
        float shifted_bin = original_bin * shift_factor; // Scaled bin index

        int bin_floor = (int)shifted_bin; // Lower integer bin
        int bin_ceil = bin_floor + 1;    // Upper integer bin
        float t = shifted_bin - bin_floor; // Fractional part for interpolation

        if (bin_floor < half_size) {
            // Linear interpolation of magnitude and phase
	  c32pluseq(&temp[bin_floor], c32mul_scalar(fft->buf[i], (1.0f - t)));
            if (bin_ceil < half_size) {
	      c32pluseq(&temp[bin_ceil], c32mul_scalar(fft->buf[i], t));
            }
        }
    }

    // Mirror the negative frequencies for real signals
    for (int i = 1; i < half_size; i++) {
        temp[fft->size - i] = c32conj(temp[i]);
    }

    // DC and Nyquist bins remain real-valued
    temp[0] = fft->buf[0]; // DC component
    temp[half_size] = fft->buf[half_size]; // Nyquist component

    // Copy the shifted spectrum back to the original buffer
    memcpy(fft->buf, temp, sizeof(temp));
}
