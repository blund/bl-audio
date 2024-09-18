#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>

#include "cadence.h"

#define internal static

typedef float Real32;

static Real32 PI = 3.14159265359;
static Real32 TAU = 2*M_PI;
static uint32_t SDL_AUDIO_TRANSPARENTLY_CONVERT_FORMAT = 0;

///////////////////////////////////////////////////////////////////////////////


// 97 -> 119;
float vals[25] = {
  60, // 97 = c
  -1, // 98
  -1, // 99
  64, // 100 = e
  63, // 101 = d#
  65, // 102 = f
  67, // 103 = g
  69, // 104 = a
  -1, // 105
  71, // 106 = b
  72, // 107 = c2
  -1, // 108
  -1, // 109
  -1, // 110
  -1, // 111
  -1, // 112
  -1, // 113
  -1, // 114
  62, // 115 = d
  66, // 116 = f#
  70, // 117 = a#
  -1, // 118
  61, // 119 = c#
  -1, // 120
  68  // 121 = g#
};

osc_t test_osc;

typedef struct program_data {
  cadence_ctx* ctx;

  synth* s;
  delay* d;
  butlp* butlp;
} program_data;

typedef struct platform_program_state
{
  program_data* data;

  bool IsRunning;
  SDL_Event LastEvent;
} platform_program_state;

typedef struct platform_audio_config
{
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
proc_loop(program_data* data)
{
  cadence_ctx* ctx = data->ctx;

  if (!ctx->initialized) {
    data->s     = new_synth(8, test_osc);
    data->d     = new_delay(ctx);
    data->butlp = new_butlp(ctx, 2000);
    data->ctx->initialized = 1;
  }

  process_gen_table(ctx);

  float sample  = play_synth(ctx, data->s);
  float delayed = apply_delay(ctx, data->d, sample, 0.3, 0.6);
  delayed       = apply_butlp(ctx, data->butlp, delayed, 200);
  
  return (int16_t)16768*(sample+delayed);
}

///////////////////////////////////////////////////////////////////////////////

internal void
SampleIntoAudioBuffer(platform_audio_buffer* AudioBuffer,
                      int16_t (*GetSample)(program_data* data),
		      program_data* data)
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
    int16_t SampleValue = (*GetSample)(data);
    *Buffer++ = SampleValue;
    *Buffer++ = SampleValue;
    AudioConfig->SampleIndex++;
  }

  Buffer = (int16_t*)AudioBuffer->Buffer;
  for (int SampleIndex = 0;
       SampleIndex < Region2Samples;
       SampleIndex++)
  {
    int16_t SampleValue = (*GetSample)(data);
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
    SampleIntoAudioBuffer(AudioThread->AudioBuffer, &proc_loop, AudioThread->ProgramState->data);
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
    "Cadence",
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


  // Set up cadence context
  ProgramState.data = malloc(sizeof(program_data));
  ProgramState.data->ctx = cadence_setup(44100);

  SDL_Event event;
  while (ProgramState.IsRunning)
  {

    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_KEYDOWN) {

	// Avoid repeats
	if (event.key.repeat) break;
	
	int sym = event.key.keysym.sym;

	// Check if program should quit
	if (sym == SDLK_q) ProgramState.IsRunning = 0;

	// Clean up the inputs
	if (sym > 121 | sym < 97) break;
	int midi_note = vals[sym-97];
	if (midi_note == -1) break;
	// Make some sound :)
	synth_register_note(ProgramState.data->s, midi_note, 0.1, NOTE_ON);
      }

      if (event.type == SDL_KEYUP) {
	int sym = event.key.keysym.sym;
	int midi_note = vals[sym-97];
	synth_register_note(ProgramState.data->s, midi_note, 0, NOTE_OFF);
      }

      if (event.type == SDL_QUIT) {
	ProgramState.IsRunning = 0;
      }
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
float test_osc(cadence_ctx* ctx, synth* s, int note_index, note* note) {
  // Variables global to all notes
  static int init = 0;

  // oscillators for each note
  static sine** sines;
  static phasor** phasors;

  // index for oscillator in gen table
  static int sine_i;

  // handle release
  static int release_samples = 5000;
  static int* release_remaining_samples;


  // initialize variables (allocate synths, initialize them, set up release
  if (!init) {
    init = 1;

    // set up local oscillators
    sines   = malloc(s->poly_count * sizeof(sine));
    phasors = malloc(s->poly_count * sizeof(phasor));
    fori(s->poly_count) sines[i] = new_sine();
    fori(s->poly_count) phasors[i] = new_phasor();


    // set up release table
    release_remaining_samples = malloc(s->poly_count * sizeof(int));
    fori(s->poly_count) release_remaining_samples[i] = 0;

    // and global ones :)
    sine_i = register_gen_table(ctx, GEN_SINE);
    ctx->gt[sine_i].p->freq = 3.0;
  }

  // handle note resets
  if (check_flag(note, NOTE_RESET)) {
    sines[note_index]->t = 0;
    phasors[note_index]->value = 0;
    release_remaining_samples[note_index] = release_samples;
    
    unset_flag(note, NOTE_RESET);
  }

  // Update phasor
  phasors[note_index]->freq = 3.0f;
  float phase = gen_phasor(ctx, phasors[note_index]);
  note->amp *= 1.0f-phase;

  // Load global modulation from gen table
  float mod = ctx->gt[sine_i].val;


  // Calculate freq for and set for generator
  sines[note_index]->freq = mtof(note->midi_note);;

  float sample = 0.2 * mod * gen_sine(ctx, sines[note_index]);

  // Manage release 
  if (check_flag(note, NOTE_RELEASE)) {
    sample *= (float)release_remaining_samples[note_index] / (float)release_samples;
    release_remaining_samples[note_index] -= 1;
    if (release_remaining_samples[note_index] == 0) {
      set_flag(note, NOTE_FREE);
      release_remaining_samples[note_index] = release_samples;
    }
  }

  return sample;
}
