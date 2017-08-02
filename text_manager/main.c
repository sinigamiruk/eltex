/*
 * Simple file manager.
 * jk - up, down
 * l - one step into
 * h - up a dir
 * d, u -- pgup, pgdwn
 * q - quit
 *
 * Scan current dir, place files into array, and print it on screen.
 */

#include "global.h"
#include "navigate.h"

int main()
{
    initscr();
    start_color();
    noecho();
    curs_set(0);
    refresh();

    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);

    status = derwin(stdscr, 1, getmaxx(stdscr), getmaxy(stdscr) - 1, 0);

    keypad(stdscr, true);
    for (int i = 0; i < WINDOW_COUNT; ++i) {
        windows[i].out = newwin(getmaxy(stdscr) - 1, getmaxx(stdscr) / WINDOW_COUNT, 0, (getmaxx(stdscr) / WINDOW_COUNT) * i);
        box(windows[i].out, 0, 0);
        wrefresh(windows[i].out);

        //setup window
        windows[i].w = derwin(windows[i].out, getmaxy(windows[i].out) - 2, getmaxx(windows[i].out) - 2 , 1, 1);
        keypad(windows[i].w, true); // key processing enabled
        strcpy(windows[i].current_path, "/home/eltex");
        windows[i].current_line = 0;
        windows[i].current_region = 0;
    }
    refresh();

    navigate();
    endwin();

    return 0;
}
