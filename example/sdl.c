#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>

#include "context.h"
#include "platform_audio.h"
#include "gen.h"
#include "synth.h"


#define internal static

typedef float Real32;

static Real32 PI = 3.14159265359;
static Real32 TAU = 2*M_PI;
static uint32_t SDL_AUDIO_TRANSPARENTLY_CONVERT_FORMAT = 0;

///////////////////////////////////////////////////////////////////////////////


float test_osc(cae_ctx* ctx, synth* s, float freq, float amp, int index, int* reset);

typedef struct platform_program_state
{
  bool IsRunning;
  SDL_Event LastEvent;
} platform_program_state;

typedef struct platform_audio_config
{
  int ToneHz;
  int ToneVolume;
  int WavePeriod;
  int SampleIndex;
  int SamplesPerSecond;
  int BytesPerSample;
} platform_audio_config;

typedef struct platform_audio_buffer
{
  uint8_t* Buffer;
  int Size;
  int ReadCursor;
  int WriteCursor;
  SDL_AudioDeviceID DeviceID;
  struct platform_audio_config* AudioConfig;
} platform_audio_buffer;

typedef struct platform_audio_thread_context
{
  platform_audio_buffer* AudioBuffer;
  platform_program_state* ProgramState;
} platform_audio_thread_context;

///////////////////////////////////////////////////////////////////////////////

internal int16_t
SampleSquareWave(platform_audio_config* AudioConfig)
{
  int HalfSquareWaveCounter = AudioConfig->WavePeriod / 2;
  if ((AudioConfig->SampleIndex / HalfSquareWaveCounter) % 2 == 0)
  {
    return AudioConfig->ToneVolume;
  }

  return -AudioConfig->ToneVolume;
}

///////////////////////////////////////////////////////////////////////////////

cae_ctx* ctx;
audio a;
sine* s;
int inited = 0;
synth* syn;
delay* d;

int n1;

internal int16_t
cae_loop(platform_audio_config* AudioConfig)
{
  if (!inited) {
    puts("initialize");
    ctx = malloc(sizeof(cae_ctx));
    a.info.sample_rate = 44100;
    ctx->a = &a;
    s = new_sine();
    s->t = 0;
    s->freq = AudioConfig->ToneHz;

    syn = new_synth(8, test_osc);
    d = new_delay(ctx);

    gen_table_init(ctx);
    inited = 1;
    puts("inited");

    synth_register_note(syn, 440.0f, 0.1, NOTE_ON, &n1);
  }

  s->freq = AudioConfig->ToneHz;
  process_gen_table(ctx);

  float sample = play_synth(ctx, syn);
  float delayed = apply_delay(ctx, d, sample, 0.3, 0.6);

  
  return (int16_t)16768*(sample+delayed);
}

///////////////////////////////////////////////////////////////////////////////

internal void
SampleIntoAudioBuffer(platform_audio_buffer* AudioBuffer,
                      int16_t (*GetSample)(platform_audio_config*))
{
  int Region1Size = AudioBuffer->ReadCursor - AudioBuffer->WriteCursor;
  int Region2Size = 0;
  if (AudioBuffer->ReadCursor < AudioBuffer->WriteCursor)
  {
    // Fill to the end of the buffer and loop back around and fill to the read
    // cursor.
    Region1Size = AudioBuffer->Size - AudioBuffer->WriteCursor;
    Region2Size = AudioBuffer->ReadCursor;
  }

  platform_audio_config* AudioConfig = AudioBuffer->AudioConfig;

  int Region1Samples = Region1Size / AudioConfig->BytesPerSample;
  int Region2Samples = Region2Size / AudioConfig->BytesPerSample;
  int BytesWritten   = Region1Size + Region2Size;

  int16_t* Buffer = (int16_t*)&AudioBuffer->Buffer[AudioBuffer->WriteCursor];
  for (int SampleIndex = 0;
       SampleIndex < Region1Samples;
       SampleIndex++)
  {
    int16_t SampleValue = (*GetSample)(AudioConfig);
    *Buffer++ = SampleValue;
    *Buffer++ = SampleValue;
    AudioConfig->SampleIndex++;
  }

  Buffer = (int16_t*)AudioBuffer->Buffer;
  for (int SampleIndex = 0;
       SampleIndex < Region2Samples;
       SampleIndex++)
  {
    int16_t SampleValue = (*GetSample)(AudioConfig);
    *Buffer++ = SampleValue;
    *Buffer++ = SampleValue;
    AudioConfig->SampleIndex++;
  }

  AudioBuffer->WriteCursor =
    (AudioBuffer->WriteCursor + BytesWritten) % AudioBuffer->Size;
}

///////////////////////////////////////////////////////////////////////////////

internal void
PlatformFillAudioDeviceBuffer(void* UserData, uint8_t* DeviceBuffer, int Length)
{
  platform_audio_buffer* AudioBuffer = (platform_audio_buffer*)UserData;

  // Keep track of two regions. Region1 contains everything from the current
  // PlayCursor up until, potentially, the end of the buffer. Region2 only
  // exists if we need to circle back around. It contains all the data from the
  // beginning of the buffer up until sufficient bytes are read to meet Length.
  int Region1Size = Length;
  int Region2Size = 0;
  if (AudioBuffer->ReadCursor + Length > AudioBuffer->Size)
  {
    // Handle looping back from the beginning.
    Region1Size = AudioBuffer->Size - AudioBuffer->ReadCursor;
    Region2Size = Length - Region1Size;
  }

  SDL_memcpy(
    DeviceBuffer,
    (AudioBuffer->Buffer + AudioBuffer->ReadCursor),
    Region1Size
  );
  SDL_memcpy(
    &DeviceBuffer[Region1Size],
    AudioBuffer->Buffer,
    Region2Size
  );

  AudioBuffer->ReadCursor =
    (AudioBuffer->ReadCursor + Length) % AudioBuffer->Size;
}

///////////////////////////////////////////////////////////////////////////////

internal void
PlatformInitializeAudio(platform_audio_buffer* AudioBuffer)
{
  SDL_AudioSpec AudioSettings = {};
  AudioSettings.freq = AudioBuffer->AudioConfig->SamplesPerSecond;
  AudioSettings.format = AUDIO_S16LSB;
  AudioSettings.channels = 2;
  AudioSettings.samples = 32;
  AudioSettings.callback = &PlatformFillAudioDeviceBuffer;
  AudioSettings.userdata = AudioBuffer;

  SDL_AudioSpec ObtainedSettings = {};
  AudioBuffer->DeviceID = SDL_OpenAudioDevice(
    NULL, 0, &AudioSettings, &ObtainedSettings,
    SDL_AUDIO_TRANSPARENTLY_CONVERT_FORMAT
  );

  if (AudioSettings.format != ObtainedSettings.format)
  {
    SDL_Log("Unable to obtain expected audio settings: %s", SDL_GetError());
    exit(1);
  }

  // Start playing the audio buffer
  SDL_PauseAudioDevice(AudioBuffer->DeviceID, 0);
}

///////////////////////////////////////////////////////////////////////////////

internal void
PlatformHandleEvent(platform_program_state* ProgramState)
{
  if (ProgramState->LastEvent.type == SDL_QUIT)
  {
    ProgramState->IsRunning = false;
  }
}

///////////////////////////////////////////////////////////////////////////////

internal int
PlatformAudioThread(void* UserData)
{
  platform_audio_thread_context* AudioThread =
    (platform_audio_thread_context*)UserData;

  while (AudioThread->ProgramState->IsRunning)
  {
    SDL_LockAudioDevice(AudioThread->AudioBuffer->DeviceID);
    SampleIntoAudioBuffer(AudioThread->AudioBuffer, &cae_loop);
    SDL_UnlockAudioDevice(AudioThread->AudioBuffer->DeviceID);
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////

int main()
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
  {
    SDL_Log("Unable to initialized SDL: %s", SDL_GetError());
    return 1;
  }

  SDL_Window* Window = SDL_CreateWindow(
    "Circular Audio Buffer Example",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    1280,
    720,
    SDL_WINDOW_OPENGL
  );

  if (!Window)
  {
    SDL_Log("Unable to initialize window: %s", SDL_GetError());
    return 1;
  }

  SDL_Renderer* Renderer = SDL_CreateRenderer(Window, -1, 0);

  platform_audio_config AudioConfig = {};
  AudioConfig.SamplesPerSecond = 44100;
  AudioConfig.BytesPerSample = 2 * sizeof(int16_t);
  AudioConfig.SampleIndex = 0;
  AudioConfig.ToneHz = 256;
  AudioConfig.ToneVolume = 3000;
  AudioConfig.WavePeriod =
    AudioConfig.SamplesPerSecond / AudioConfig.ToneHz;

  platform_audio_buffer AudioBuffer = {};
  AudioBuffer.Size = 2048; //AudioConfig.SamplesPerSecond * AudioConfig.BytesPerSample; // 2048

  AudioBuffer.Buffer = malloc(sizeof(uint8_t)*AudioBuffer.Size);
  // Two data points per sample. One for the left speaker, one for the right.
  AudioBuffer.ReadCursor = 0;
  // NOTE: Offset by 1 sample in order to cause the circular buffer to
  // initially be filled.
  AudioBuffer.WriteCursor = AudioConfig.BytesPerSample;
  AudioBuffer.AudioConfig = &AudioConfig;
  memset(AudioBuffer.Buffer, 0, AudioBuffer.Size);

  PlatformInitializeAudio(&AudioBuffer);

  platform_program_state ProgramState = {};
  ProgramState.IsRunning = true;

  platform_audio_thread_context AudioThreadContext = {};
  AudioThreadContext.AudioBuffer = &AudioBuffer;
  AudioThreadContext.ProgramState = &ProgramState;
  SDL_Thread* AudioThread = SDL_CreateThread(
    PlatformAudioThread, "Audio", (void*)&AudioThreadContext
  );


 
  while (ProgramState.IsRunning)
  {
    while (SDL_PollEvent(&ProgramState.LastEvent))
    {
      switch( ProgramState.LastEvent.type ){
	/* Keyboard event */
	/* Pass the event data onto PrintKeyInfo() */
      case SDL_KEYDOWN: {
	SDL_Event event = ProgramState.LastEvent;
	switch( event.key.keysym.sym ){
	case SDLK_a:
	  puts("epic");
	  AudioConfig.ToneHz = 330;
	  AudioConfig.WavePeriod =
	    AudioConfig.SamplesPerSecond / AudioConfig.ToneHz;
	  break;
	case SDLK_b:
	  puts("epic");
	  AudioConfig.ToneHz = 440;
	  AudioConfig.WavePeriod =
	    AudioConfig.SamplesPerSecond / AudioConfig.ToneHz;
	  break;
	case SDLK_q:
	  ProgramState.IsRunning = 0;
	  break;
	}
      }
      case SDL_KEYUP:
	break;
      case SDL_QUIT:
	ProgramState.IsRunning = 0;
	break;

      default:
	break;
      }
      PlatformHandleEvent(&ProgramState);
    }

    SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 0);
    SDL_RenderClear(Renderer);
    SDL_RenderPresent(Renderer);
  }

  SDL_WaitThread(AudioThread, NULL);

  SDL_DestroyRenderer(Renderer);
  SDL_DestroyWindow(Window);
  SDL_CloseAudioDevice(AudioBuffer.DeviceID);
  SDL_Quit();

  return 0;
}


// Test osc to demonstrate polyphony
float test_osc(cae_ctx* ctx, synth* s, float freq, float amp, int index, int* reset) {
  static sine** sines;
  static phasor** phasors;;
  static int init = 0;
  static int sine_i;

  if (!init) {
    init = 1;

    // set up local oscillators
    sines   = malloc(s->poly_count * sizeof(sine));
    phasors = malloc(s->poly_count * sizeof(phasor));
    fori(4) sines[i] = new_sine();
    fori(4) phasors[i] = new_phasor();

    // and global ones :)
    sine_i = register_gen_table(ctx, GEN_SINE);
    ctx->gt[sine_i].p->freq = 0.8;
  }

  if (*reset) {
    sines[index]->t = 0;
    phasors[index]->value = 0;
    *reset = 0;
  }

  phasors[index]->freq = 1.0f;
  float phase = gen_phasor(ctx, phasors[index]);
  amp *= 1.0f-phase;

  float mod = ctx->gt[sine_i].val;

  sines[index]->freq = freq;// + 15.0*mod;
  float sample = amp * gen_sine(ctx, sines[index]);
  return sample;
}
