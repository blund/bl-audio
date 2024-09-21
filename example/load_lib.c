/*****************************************************************
 *  load_lib.c                                                    *
 *  Created on 28.06.23                                           *
 *  By Børge Lundsaunet                                           *
 *     - big simplification 21.09.24                              *
 *****************************************************************/

#include <malloc.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "load_lib.h"

time_t get_write_time(const char* lib_path) {
    struct stat attr;

    if (stat(lib_path, &attr) == 0) {
        return attr.st_mtime;
    } 

    perror("stat() failed");
    return 0;
}


// Define program so path and what functions to load
void load_functions(library* lib) {
  char* lib_path = "/home/blund/prosjekt/personlig/cadence/example/program.so";
  char* lib_done = "/home/blund/prosjekt/personlig/cadence/example/program.so.done";

  // Handle initial load
  if (lib->handle == 0) {
    lib->handle = dlopen(lib_path, RTLD_NOW | RTLD_GLOBAL);
    lib->last_write_time = get_write_time(lib_done);
    goto load_funs;
  }

  // Check current lib write time
  time_t current_write_time = get_write_time(lib_done);

  // If write time is different from last, reload lib
  if (lib->last_write_time != current_write_time) {
    dlclose(lib->handle);
    lib->handle = dlopen(lib_path, RTLD_NOW | RTLD_GLOBAL);
    lib->last_write_time = current_write_time;
    goto load_funs;
  }
  return;

 load_funs:
  lib->functions.program_loop = (program_loop_t*)dlsym(lib->handle, "program_loop");
  lib->functions.midi_event   = (midi_event_t*)dlsym(lib->handle,   "midi_event");
  lib->functions.draw_gui     = (draw_gui_t*)dlsym(lib->handle,     "draw_gui");
  return;
}
