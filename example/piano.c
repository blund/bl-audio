/* nuklear - 1.32.0 - public domain */
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <pthread.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT

#include <nuklear/nuklear.h>
#include <nuklear/nuklear_glfw_gl3.h>

#include "../cadence.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#ifdef INCLUDE_ALL
  #define INCLUDE_STYLE
  #define INCLUDE_CALCULATOR
  #define INCLUDE_CANVAS
  #define INCLUDE_OVERVIEW
  #define INCLUDE_NODE_EDITOR
#endif

#ifdef INCLUDE_STYLE
  #include "../../demo/common/style.c"
#endif
#ifdef INCLUDE_CALCULATOR
  #include "../../demo/common/calculator.c"
#endif
#ifdef INCLUDE_CANVAS
  #include "../../demo/common/canvas.c"
#endif
#ifdef INCLUDE_OVERVIEW
  #include "../../demo/common/overview.c"
#endif
#ifdef INCLUDE_NODE_EDITOR
  #include "../../demo/common/node_editor.c"
#endif

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/


pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *audio_thread(void *vargp) 
{

  int* play = (int*)vargp;

  // Store the value argument passed to this thread 
  cae_ctx* ctx = malloc(sizeof(cae_ctx));
  platform_audio_setup(ctx);

  
  
  sine* s = new_sine();

  for (;;) {
  s->freq = *play;
    fori(ctx->a->info.frames) {
      float sample = gen_sine(ctx, s);
      write_to_track(ctx, 0, i, sample);
    }
    mix_tracks(ctx);
    platform_audio_play_buffer(ctx);
  }

  platform_audio_cleanup(ctx);
  return 0;
}

static void error_callback(int e, const char *d)
{printf("Error %d: %s\n", e, d);}

int main(void)
{
  pthread_t tid;

  int play = 330;

  int rc;
  pthread_attr_t attr;
  struct sched_param param;
  rc = pthread_attr_init (&attr);
  rc = pthread_attr_getschedparam (&attr, &param);
  (param.sched_priority)++;
  rc = pthread_attr_setschedparam (&attr, &param);
  pthread_create(&tid, &attr, audio_thread, (void *)&play);

  /* Platform */
  struct nk_glfw glfw = {0};
  static GLFWwindow *win;
  int width = 0, height = 0;
  struct nk_context *ctx;
  struct nk_colorf bg;

  /* GLFW */
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) {
    fprintf(stdout, "[GFLW] failed to init!\n");
    exit(1);
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Demo", NULL, NULL);
  glfwMakeContextCurrent(win);
  glfwGetWindowSize(win, &width, &height);

  /* OpenGL */
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glewExperimental = 1;
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to setup GLEW\n");
    exit(1);
  }

  ctx = nk_glfw3_init(&glfw, win, NK_GLFW3_INSTALL_CALLBACKS);
  {struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&glfw, &atlas);
    /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
    /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 14, 0);*/
    /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
    /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
    /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
    /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
    nk_glfw3_font_stash_end(&glfw);
    /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
    /*nk_style_set_font(ctx, &droid->handle);*/}

  bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
  while (!glfwWindowShouldClose(win))
    {
      /* Input */
      glfwPollEvents();
      nk_glfw3_new_frame(&glfw);

      /* GUI */
      if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
		   NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
		   NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
	  nk_layout_row_static(ctx, 30, 80, 1);
	  if (nk_button_label(ctx, "button")) {
	    //pthread_mutex_lock(&m);
	    play = 440;
	    //pthread_mutex_unlock(&m);
	  }

	  /*
	  enum {EASY, HARD};
	  static int op = EASY;
	  static int property = 20;
	  nk_layout_row_static(ctx, 30, 80, 1);
	  
	  nk_layout_row_dynamic(ctx, 30, 2);
	  if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
	  if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

	  nk_layout_row_dynamic(ctx, 25, 1);
	  nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

	  nk_layout_row_dynamic(ctx, 20, 1);
	  nk_label(ctx, "background:", NK_TEXT_LEFT);
	  nk_layout_row_dynamic(ctx, 25, 1);
	  if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx),400))) {
	    nk_layout_row_dynamic(ctx, 120, 1);
	    bg = nk_color_picker(ctx, bg, NK_RGBA);
	    nk_layout_row_dynamic(ctx, 25, 1);
	    bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
	    bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
	    bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
	    bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
	    nk_combo_end(ctx);
	  }
	  */
        }
      nk_end(ctx);

      /* -------------- EXAMPLES ---------------- */
#ifdef INCLUDE_CALCULATOR
      calculator(ctx);
#endif
#ifdef INCLUDE_CANVAS
      canvas(ctx);
#endif
#ifdef INCLUDE_OVERVIEW
      overview(ctx);
#endif
#ifdef INCLUDE_NODE_EDITOR
      node_editor(ctx);
#endif
      /* ----------------------------------------- */

      /* Draw */
      glfwGetWindowSize(win, &width, &height);
      glViewport(0, 0, width, height);
      glClear(GL_COLOR_BUFFER_BIT);
      glClearColor(bg.r, bg.g, bg.b, bg.a);
      /* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
       * with blending, scissor, face culling, depth test and viewport and
       * defaults everything back into a default state.
       * Make sure to either a.) save and restore or b.) reset your own state after
       * rendering the UI. */
      nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
      glfwSwapBuffers(win);
    }
  nk_glfw3_shutdown(&glfw);
  glfwTerminate();
  return 0;
}
