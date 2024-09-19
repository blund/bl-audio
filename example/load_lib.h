/******************************************************************
 *  load_lib.h                                                    *
 *  Created on 28.06.23                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/


#pragma once

#include <bits/types/time_t.h>
#include <sys/stat.h>

#include "library.h"

time_t get_last_write(const char* lib_path);

void set_lockfile(char* new_lockfile);
void set_tmp_path(char* new_tmp_path);

int load_lib(library* lib, const char* lib_path);
void unload_lib(library* lib);

void wait_for_lock();
