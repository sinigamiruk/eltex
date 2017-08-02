/*
 * Copy submodule. Use some window.
 */
#ifndef COPY_H
#define COPY_H

#include <pthread.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

// Copy file from one path to another, printing status into window
// taken with 3 parametr.
void copy(char *source, char * destination, WINDOW *win);

#endif
