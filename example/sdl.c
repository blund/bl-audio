/*****************************************************************
 *  sdl.c                                                         *
 *  Created on 18.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/



#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <alsa/asoundlib.h>

#include <stdint.h>
#include <stdbool.h>

#include <dlfcn.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_gl3.h"

#undef NK_IMPLEMENTATION
#include "library.h"
#include "load_lib.h"

#include "cadence.h"
#include "midi.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 500

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

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

library* lib;

snd_rawmidi_t *midi_in;
int status;

typedef struct platform_program_state
{
  void* program_data;

  bool      IsRunning;
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
  program_loop_t* program_loop;
} platform_audio_thread_context;

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

internal void
SampleIntoAudioBuffer(platform_audio_buffer* AudioBuffer,
		      program_loop_t program_loop,
                      //int16_t (*GetSample)(void* data),
		      void* data)
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
  for (int SampleIndex = 0; SampleIndex < Region1Samples; SampleIndex++) {
    int16_t SampleValue = (*program_loop)(data);
    *Buffer++ = SampleValue;
    *Buffer++ = SampleValue;
    AudioConfig->SampleIndex++;
  }

  Buffer = (int16_t*)AudioBuffer->Buffer;
  for (int SampleIndex = 0; SampleIndex < Region2Samples; SampleIndex++) {
    int16_t SampleValue = (*program_loop)(data);
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

  int counter = 0;
  while (AudioThread->ProgramState->IsRunning)  {
    SDL_LockAudioDevice(AudioThread->AudioBuffer->DeviceID);
    SampleIntoAudioBuffer(AudioThread->AudioBuffer, lib->functions.program_loop, AudioThread->ProgramState->program_data);
    SDL_UnlockAudioDevice(AudioThread->AudioBuffer->DeviceID);
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  /* Platform */
  SDL_Window *win;
  SDL_GLContext glContext;
  int win_width, win_height;

  /* GUI */
  struct nk_context *ctx;
  struct nk_colorf bg;


  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
      SDL_Log("Unable to initialized SDL: %s", SDL_GetError());
      return 1;
  }

  
  //SDL_Renderer* Renderer = SDL_CreateRenderer(Window, -1, 0);


  /* Nuklear code */
  
  NK_UNUSED(argc);
  NK_UNUSED(argv);

  /* SDL setup */
  SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  if (!win) {
    SDL_Log("Unable to initialize window: %s", SDL_GetError());
    return 1;
  }
  win = SDL_CreateWindow("Cadence",
			 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			 WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI);
  glContext = SDL_GL_CreateContext(win);
  SDL_GetWindowSize(win, &win_width, &win_height);


  /* OpenGL setup */
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glewExperimental = 1;
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to setup GLEW\n");
    exit(1);
  }

  ctx = nk_sdl_init(win);
  /* Load Fonts: if none of these are loaded a default font will be used  */
  /* Load Cursor: if you uncomment cursor loading please hide the cursor */
  {struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
    /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16, 0);*/
    /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
    /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
    /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
    /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
    nk_sdl_font_stash_end();
    /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
    /*nk_style_set_font(ctx, &roboto->handle);*/}

  /* End of Nuklear code */


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

  // Load library functions
  lib = malloc(sizeof(library));
  lib->handle    = 0; // Ensure that malloc makes this val 0 :)
  lib->path      = "program.so";
  lib->done_file = "program.so.done";
  load_functions(lib);

  
  platform_audio_thread_context AudioThreadContext = {};
  AudioThreadContext.AudioBuffer = &AudioBuffer;
  AudioThreadContext.ProgramState = &ProgramState;
  SDL_Thread* AudioThread = SDL_CreateThread(
    PlatformAudioThread, "Audio", (void*)&AudioThreadContext
  );

  SDL_Event event;
  int counter = 0;

  int midi_enabled = 1;

  // Load midi controller
  status = snd_rawmidi_open(&midi_in, NULL, "hw:2,0,0", SND_RAWMIDI_NONBLOCK);
  if (status < 0) {
    midi_enabled = 0;
    //fprintf(stderr, "Error opening MIDI input: %s\n", snd_strerror(status));
    puts("Note: Not using a midi device");
  }

  char midi_buffer[3];
  static Midi_Note midi; // @NOTE - for some reason this has to be static..

  while (ProgramState.IsRunning)
  {
    counter += 1;
    if (counter == 100) {
      load_functions(lib);
      counter = 0;
    }

    if (midi_enabled) {
      int status = snd_rawmidi_read(midi_in, midi_buffer, 3);
      if (status > 0) {
	read_midi_note_from_buf(midi_buffer, &midi);
	if (midi.message == MIDI_NOTE_ON) {
	  puts(" -- note on");
	  printf("Note: %x, Vel: %x\n", midi.note, midi.vel);
	  lib->functions.midi_event(0, midi.note, (float)midi.vel/127, NOTE_ON);
	}
	if (midi.message == MIDI_NOTE_OFF) {
	  puts(" -- note off");
	  printf("Note: %x, Vel: %x\n", midi.note, midi.vel);
	  lib->functions.midi_event(0, midi.note, (float)midi.vel/127, NOTE_OFF);
	}
	if (midi.message == MIDI_CONTROL_CHANGE) {
	  puts(" -- slider");
	  printf("Controller: %x, Val: %x\n", midi.controller, midi.val);
	}
      }
    }


    nk_input_begin(ctx);
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_KEYDOWN) {

	// Avoid repeats
	if (event.key.repeat) break;
	
	int sym = event.key.keysym.sym;

	// Check if program should quit
	if (sym == SDLK_q) ProgramState.IsRunning = 0;

	// Clean up the inputs
	if (sym > 121 | sym < 97) continue;
	int midi_note = vals[sym-97];
	if (midi_note == -1) break;

	// registrer event, med synth 0, midi-note, vel og note-on
	lib->functions.midi_event(0, midi_note, 0.1, NOTE_ON);
      }

      if (event.type == SDL_KEYUP) {
	int sym = event.key.keysym.sym;
	if (sym > 121 | sym < 97) continue;
	int midi_note = vals[sym-97];
	// registrer event, med synth 0, midi-note, vel og note-on
	lib->functions.midi_event(0, midi_note, 0.1, NOTE_OFF);
      }



      
      if (event.type == SDL_QUIT) {
	ProgramState.IsRunning = 0;

	goto cleanup;
      }

      nk_sdl_handle_event(&event);
    }

    
    /* Draw */
    nk_sdl_handle_grab(); /* optional grabbing behavior */
    nk_input_end(ctx);
    lib->functions.draw_gui(ctx);
    nk_end(ctx);

    SDL_GetWindowSize(win, &win_width, &win_height);
    glViewport(0, 0, win_width, win_height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(bg.r, bg.g, bg.b, bg.a);
    /* IMPORTANT: `nk_sdl_render` modifies some global OpenGL state
     * with blending, scissor, face culling, depth test and viewport and
     * defaults everything back into a default state.
     * Make sure to either a.) save and restore or b.) reset your own state after
     * rendering the UI. */
    nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
    SDL_GL_SwapWindow(win);

  }

 cleanup:
  nk_sdl_shutdown();

  SDL_WaitThread(AudioThread, NULL);

  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(win);
  SDL_CloseAudioDevice(AudioBuffer.DeviceID);
  SDL_Quit();

  return 0;
}


