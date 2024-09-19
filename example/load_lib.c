/*****************************************************************
 *  load_lib.c                                                    *
 *  Created on 28.06.23                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/


#include <dlfcn.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/sendfile.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <malloc.h>

#include "load_lib.h"

time_t get_last_write(const char* lib_path) {
    const char* filename = lib_path;
    struct stat fileStat;

    if (stat(filename, &fileStat) == 0) {
        return fileStat.st_ctime;
    } 

    perror("stat() failed");
    return 0;
}

int copy_file(const char* source, const char* destination) {    
    int input = open(source, O_RDONLY);
    if (input == -1) {
      perror("Could not open source file for copy");
        return -1;
    }

    int output = creat(destination, 0660);
    if (output == -1) {
      perror("Could not open destionation file for copy");
      close(input);
      return -1;
    }

    // Copy file with 'sendfile'
    off_t bytesCopied = 0;
    struct stat fileinfo = {};
    fstat(input, &fileinfo);
    int result = sendfile(output, input, &bytesCopied, (size_t)fileinfo.st_size);

    close(input);
    close(output);

    return result;
}

char* lockfile;
char* tmp_path;

void set_lockfile(char* new_lockfile) {
  lockfile = new_lockfile;
}

void set_tmp_path(char* new_tmp_path) {
  tmp_path = new_tmp_path;
}



void wait_for_lock() {
  int is_locked = 0;
  while(is_locked != -1) {
    is_locked = open(lockfile, O_RDONLY);
    close(is_locked);
  }
}


int load_lib(library* lib, const char* lib_path) {
  int res = copy_file(lib_path, tmp_path);
  if (res == -1) {
    printf("Could not create temporary copy of '%s'\n", lib_path);
    return 0;
  }

  void* handle = dlopen(tmp_path, RTLD_LAZY);

  if (!handle) {
    printf("Could not load library '%s'\n", lib_path);
    return 0;
  }

  // Set handle for reference.
  lib->handle = handle;

  // Set write time for future reload
  lib->last_write = get_last_write(lib_path);
  return 1;
}
