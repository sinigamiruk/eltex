#ifndef GLOBAL_CONFIG
#define GLOBAL_CONFIG

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

#include <ncurses.h>
#include <sys/types.h>
#include <dirent.h>

#define WINDOW_COUNT 2

struct field {
    WINDOW *out;    // outter window, to draw a box
    WINDOW *w;      // inner  window, to print files
    char current_path[2048];
    int current_region; // describes region, with start in this item, thats
    //may have lenght as screen line numbers
    int current_line; // currently selected line in region, must be
    // between 0 and getmaxy(window)
};

struct field windows[WINDOW_COUNT];
WINDOW *status;


#endif
