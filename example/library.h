/*****************************************************************
 *  library.h                                                     *
 *  Created on 19.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/



#ifndef CADENCE_LIBRARY_H
#define CADENCE_LIBRARY_H

#include "stdbool.h"
#include "time.h"

#include "program.h"

typedef struct library_functions {
  program_loop_t* program_loop;
  midi_event_t*   midi_event;
  draw_gui_t*     draw_gui;
} library_functions;

typedef struct library {
  bool is_initialized;

  void* handle;
  time_t last_write_time;

  library_functions functions;
} library;

#endif
