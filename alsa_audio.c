
#include "context.h"

void platform_audio_setup(cae_ctx* ctx) {
  ctx->a = malloc(sizeof(audio));
  audio* a = ctx->a;

  // clear out channel buffers
  for (int n = 0; n < num_tracks; n++) {
    for (int i = 0; i < track_size; i++) {
      a->tracks[n][2*i]   = 0;
      a->tracks[n][2*i+1] = 0;
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

  a->info.rc = rc;
  a->info.handle = handle;
  a->info.params = params;
  a->info.sample_rate = sample_rate;
  a->info.dir = dir;
  a->info.frames = frames;
  a->info.buffer = buffer;
  a->info.buffer_size = buffer_size;
}


void platform_audio_play_buffer(cae_ctx* ctx) {
  audio_info* a = &ctx->a->info;

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

void platform_audio_cleanup(cae_ctx* ctx) {
  snd_pcm_drain(ctx->a->info.handle);
  snd_pcm_close(ctx->a->info.handle);
}
