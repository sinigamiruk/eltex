#include "ui.h"

void total_refresh()
{
    for (int i = 0; i < WINDOW_COUNT; ++i){
        box(windows[i].out, 0, 0);
        wrefresh(windows[i].out);
        wrefresh(windows[i].w);
        struct dirent * d[2048];
        print_files(i, d);
    }
    refresh();
}

int max(int a, int b)
{
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

int popup(char *message)
{
    int width = max(strlen(message) + 4, 13); // 13 - window's width with 'yes' and 'no' buttons

    WINDOW *p = derwin(stdscr, 7, width,  // size : 7 rows and width to message
            (getmaxy(stdscr) - 7) / 2, (getmaxx(stdscr) - width) / 2);

    wattron(p, COLOR_PAIR(2));
    wbkgd(p, COLOR_PAIR(2));
    mvwprintw(p, 2, (width - strlen(message)) / 2 , "%s\n", message); // centering message
    keypad(p, true);

    int current_button = 1;

    do {
        if (current_button) {
            wattron(p, COLOR_PAIR(1));
            mvwprintw(p, 4, (width) / 2 - 3, "yes");
            wattroff(p, COLOR_PAIR(1));
            mvwprintw(p, 4, (width) / 2 + 1, "no");
        } else {
            mvwprintw(p, 4, (width) / 2 - 3, "yes");
            wattron(p, COLOR_PAIR(1));
            mvwprintw(p, 4, (width) / 2 + 1, "no");
            wattroff(p, COLOR_PAIR(1));
        }
        box(p, 0, 0);
        wrefresh(p);

        char c = wgetch(p);
        switch(c) {
            case 9: //tab
                current_button = (current_button + 1) % 2;
            break;

            case 10: // enter
                wattroff(p, COLOR_PAIR(2));
                total_refresh();
                return current_button;
            break;
            default: continue;
        }
    } while(1);
    return 0;
}
