

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <math.h>

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


#define num_channels 4
#define channel_size 64

typedef struct audio {
  audio_info info;
  float channels[num_channels][channel_size];
} audio;


void audio_setup(audio** a);
void audio_cleanup(audio_info* a);
void audio_play_buffer(audio_info* a);


// declare global audio a so we don't have to pass it around :)
audio* a;

// -- sine --
typedef struct sine {
  float t;
  float freq;
  float volume;
} sine;

sine* new_sine(float freq, float volume) {
  sine* s = (sine*)malloc(sizeof(sine)); // @NOTE - hardcoded max buffer size
  s->t = 0;
  s->freq = freq;
  s->volume = volume;
  return s;
}

float gen_sine(sine* sine) {
  float   wave_period = a->info.sample_rate / sine->freq;
  float   sample      = sine->volume * sinf(sine->t);

  sine->t += 2.0f * M_PI * 1.0f / wave_period;
  return sample;
}


// -- phasor --
typedef struct phasor {
  float freq;
  float value;
} phasor;

phasor* new_phasor(float freq) {
  phasor* p = (phasor*)malloc(sizeof(phasor)); // @NOTE - hardcoded max buffer size
  p->freq = freq;
  p->value = 0;
  return p;
}

float gen_phasor(phasor* p) {
  double diff =  (double)p->freq / (double)a->info.sample_rate;
  p->value = (double)p->value + diff;

  if (p->value >= 1.0f) p->value = -1.0f;

  return p->value;
}


// -- delay --
typedef struct delay {
  float    buffer[10*44100];
  uint32_t buf_size;
  uint32_t write_head;
  uint32_t read_offset;
  float    feedback;
} delay;

delay* new_delay(float delay_ms, float feedback) {
  delay* d = (delay*)malloc(sizeof(delay)); // @NOTE - hardcoded max buffer size
  d->buf_size = 10*44100;

  // clear out buffer @NOTE - might be unecessary
  for (int i = 0; i < d->buf_size; i++) {
    d->buffer[i] = 0;
  }

  d->read_offset = delay_ms * 44100; // @NOTE - hardcoded samplerate
  d->feedback = feedback;
  return d;
}

float apply_delay(delay* d, float sample) {
  // read from delay buffer
  uint32_t index = (d->write_head - d->read_offset) % d->buf_size;
  float delayed  = d->buffer[index];

  // write back + feedback!
  d->buffer[d->write_head % d->buf_size] = sample + delayed * d->feedback;

  // increment delay
  d->write_head++;
  return delayed;
}


int main() {
  printf("cadence audio engine v. 0.1\n");

  audio_setup(&a);
 
  sine* s1 = new_sine(330, 0.1);
  sine* s2 = new_sine(440, 0.1);

  phasor* p1 = new_phasor(330);
  phasor* p2 = new_phasor(331);

  delay* d1 = new_delay(0.5, 0.3);
  delay* d2 = new_delay(0.3, 0.3);


  for (int j = 0; j < 1024*8; j++) {
    // do processing pr sample pr frame
    for (int i = 0; i < a->info.frames; i++) {
      if (j >= 1024)
      {
	float sample  = gen_sine(s1);
	float delayed = apply_delay(d1, sample);
	a->channels[0][i] = sample + delayed;
      }

      if (1) {
	float sample  = gen_sine(s2);
	float delayed = apply_delay(d2, sample);
	a->channels[1][i] = sample + delayed;
      }
      
      if (1) {
	float sample1 = gen_phasor(p1);
	a->channels[2][i] = 0.05*sample1;
      }

      if (1) {
	float sample = gen_phasor(p2);
	a->channels[3][i] = 0.05*sample;
      }
    }
    
    // mix channels
    // for every sample in each frame..
    for (int i = 0; i < a->info.frames; i++) {

      float sample = 0;
      // ...sum up each channel :)
      for (int n = 0; n < num_channels; n++) {
	sample += a->channels[n][i];
      }

      // sample to 16 bit int and copy to alsa buffer
      a->info.buffer[i] = (int16_t)(32768.0f * sample);
    }

    // buffer playback
    audio_play_buffer(&a->info);
  }

  // cleanup
  audio_cleanup(&a->info);
}


void audio_setup(audio** a) {

  *a = malloc(sizeof(audio));

  // clear out channel buffers
  for (int n = 0; n < num_channels; n++) {
    for (int s = 0; s < channel_size; s++) {
      (*a)->channels[n][s] = 0;
    }
  }
 

  int rc;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int sample_rate = 44100;
  int dir;
  snd_pcm_uframes_t frames = 32;
  int16_t *buffer;
  int buffer_size;
    
  // Open PCM device for playback.
  rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0) {
    fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
    exit(1);
  }

  // Allocate a hardware parameters object.
  snd_pcm_hw_params_malloc(&params);

  // Fill it in with default values.
  snd_pcm_hw_params_any(handle, params);

  // Set the desired hardware parameters.

  // Interleaved mode
  snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

  // Signed 16-bit little-endian format
  snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

  // Two channels (stereo)
  snd_pcm_hw_params_set_channels(handle, params, 1);

  // 44100 bits/second sampling rate (CD quality)
  snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, &dir);

  // Set period size to 32 frames.
  snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

  // Write the parameters to the driver
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
    exit(1);
  }

  // Use a buffer large enough to hold one period
  snd_pcm_hw_params_get_period_size(params, &frames, &dir);
  buffer_size = frames; // 2 bytes/sample, 2 channels
  buffer = (int16_t *)malloc(buffer_size*sizeof(int16_t));

  for (int i = 0; i < buffer_size; i++) {
    buffer[i] = 0;
  }

  (*a)->info.rc = rc;
  (*a)->info.handle = handle;
  (*a)->info.params = params;
  (*a)->info.sample_rate = sample_rate;
  (*a)->info.dir = dir;
  (*a)->info.frames = frames;
  (*a)->info.buffer = buffer;
  (*a)->info.buffer_size = buffer_size;
}

void audio_cleanup(audio_info* a) {
  snd_pcm_drain(a->handle);
  snd_pcm_close(a->handle);
}

void audio_play_buffer(audio_info* a) {
  a->rc = snd_pcm_writei(a->handle, a->buffer, a->frames);
  if (a->rc == -EPIPE) {
    // EPIPE means underrun
    fprintf(stderr, "underrun occurred\n");
    snd_pcm_prepare(a->handle);
  } else if (a->rc < 0) {
    fprintf(stderr, "error from writei: %s\n", snd_strerror(a->rc));
  } else if (a->rc != (int)a->frames) {
    fprintf(stderr, "short write, write %d frames\n", a->rc);
  }
}
