
/*
  Cadence Audio Engine
  May 2024, Roma, Italy
  Børge Lundsaunet
*/

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include <time.h>

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


#define num_tracks 4
#define track_size 128
#define channels_pr_track 2

typedef struct audio {
  audio_info info;
  float tracks[num_tracks][channels_pr_track*track_size];
} audio;


void audio_setup(audio** a);
void audio_cleanup(audio_info* a);
void audio_play_buffer(audio_info* a);



float sigmoid(float x) {
  x *= 0.1;
  return 1 / ( 1+exp(-x*5.2f));
}

// declare global audio a so we don't have to pass it around :)
audio* a;

void write_to_track(int n, int i, float sample) {
  a->tracks[n][2*i]   = sample;
  a->tracks[n][2*i+1] = sample;
}

// -- sine --
typedef struct sine {
  double t;
} sine;

sine* new_sine() {
  sine* s = (sine*)malloc(sizeof(sine)); // @NOTE - hardcoded max buffer size
  s->t = 0;
  return s;
}

float gen_sine(sine* sine, float freq, float volume) {
  float   wave_period = a->info.sample_rate / freq;
  float   sample      = volume * sin(sine->t);

  sine->t += 2.0f * M_PI * 1.0f / wave_period;

  sine->t = fmod(sine->t, 2.0f*M_PI);
  return sample;
}


// -- phasor --
typedef struct phasor {
  double freq;
  double value;
} phasor;

phasor* new_phasor(float freq) {
  phasor* p = (phasor*)malloc(sizeof(phasor)); // @NOTE - hardcoded max buffer size
  p->freq = freq;
  p->value = 0;
  return p;
}

float gen_phasor(phasor* p) {
  double diff =  (double)p->freq / (double)a->info.sample_rate;
  p->value += diff;
  if (p->value > 1.0f) p->value -= 2.0f;
  return p->value;
}

float apply_amp(float amp, float sample) {
  return amp * sample;
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
  return sample + delayed*d->feedback;
}


int main(int argc, char** argv) {

  int perf_mode = 0;
  if (argc == 2) {
    printf("argv: %s\n", argv[1]);
    perf_mode = 1;
  }

  printf("cadence audio engine v0.1\n");

  if (!perf_mode) {
    audio_setup(&a);
  } else {
    a = malloc(sizeof(audio));
    a->info.sample_rate = 44100;
    a->info.frames = 256;
    a->info.buffer = malloc(512*2*sizeof(int16_t));
  }
  sine* s1  = new_sine();
  sine* s2  = new_sine();
  sine* s3  = new_sine();

  phasor* p1 = new_phasor(330);
  phasor* p2 = new_phasor(330);
  phasor* p3 = new_phasor(330);

  delay* d1  = new_delay(0.4, 0.7);
  delay* d2  = new_delay(0.3, 0.6);


  printf("%d\n", a->info.frames);
  for (int j = 0; j < 1024*8; j++) {
    clock_t begin = clock(); // to time how long it takes to render sound

    for (int i = 0; i < a->info.frames; i++) {
      {
	float mod = gen_sine(s2, 8.0, 80.0f);

	float sample = gen_sine(s1, 440 + mod, 0.3);
	sample *= 0.5;
	float delayed = apply_delay(d1, sample);
	write_to_track(0, i, sample + delayed);
      }

      {
	float sample = gen_phasor(p3);
	sample *= 0.1;
	//write_to_track(2, i, sample);
      }
    }
 

    // end render timer
    clock_t end = clock();
    double render_time = (double)(end - begin) / CLOCKS_PER_SEC;

    // begin playback timer
    begin = clock();

    // mix tracks
    // for every sample in each frame..
    for (int i = 0; i < a->info.frames; i++) {
      float sample_l = 0;
      float sample_r = 0;

      // ...sum up each channel :)
      for (int n = 0; n < num_tracks; n++) {
	sample_l += a->tracks[n][2*i];
	sample_r += a->tracks[n][2*i+1];
      }

      // sample to 16 bit int and copy to alsa buffer
      a->info.buffer[2*i]   = (int16_t)(32768.0f * sample_l);
      a->info.buffer[2*i+1] = (int16_t)(32768.0f * sample_r);
    }

    // buffer playback
    if (!perf_mode) audio_play_buffer(&a->info);

    // end playback timer
    end = clock();
    double play_time = (double)(end - begin) / CLOCKS_PER_SEC;

    // display timer infos
    double max_time = ((double)track_size/2/(double)44100);
    printf("total:              %f\n", max_time);
    printf("finish render sound %f\n", render_time);
    printf("finish play sound   %f\n", play_time);
    printf("remaining:          %f\n\n", max_time-render_time-play_time);
    assert(max_time-render_time-play_time> 0.0f);
  }

  // cleanup
  if (!perf_mode) audio_cleanup(&a->info);
}


void audio_setup(audio** a) {
  *a = malloc(sizeof(audio));

  // clear out channel buffers
  for (int n = 0; n < num_tracks; n++) {
    for (int i = 0; i < track_size; i++) {
      (*a)->tracks[n][2*i]   = 0;
      (*a)->tracks[n][2*i+1] = 0;
    }
  }

  int rc;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params=NULL;
  snd_pcm_sw_params_t *sw=NULL;
  unsigned int sample_rate = 44100;
  int dir;
  snd_pcm_uframes_t frames = track_size/2-1;
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

  // Two tracks (stereo)
  snd_pcm_hw_params_set_channels(handle, params, 2);

  // 44100 bits/second sampling rate (CD quality)
  snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, &dir);

  // Set period size to 64 frames.
  snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

  // Write the parameters to the driver
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
    exit(1);
  }

  // allocate software params
  snd_pcm_sw_params_alloca(&sw);
  snd_pcm_sw_params_current(handle, sw);

  // start playing after the first write
  snd_pcm_sw_params_set_start_threshold(handle, sw, 1);

  // wake up on every interrupt
  snd_pcm_sw_params_set_avail_min(handle, sw, 1);

  // write config to driver
  snd_pcm_sw_params(handle, sw);

  // Use a buffer large enough to hold one period
  snd_pcm_hw_params_get_period_size(params, &frames, &dir);
  buffer_size = frames*2; // 2 bytes/sample, 2 tracks
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
