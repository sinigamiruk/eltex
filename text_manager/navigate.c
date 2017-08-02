#include "navigate.h"

int min(int a, int b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}
// Process keypresses, and what to do with it.
void navigate()
{
    int c;
    int current_window = 0; // what panel, left or right, is focused

    struct dirent *d[256];
    int file_count = 0;

    for (int i = 0; i < WINDOW_COUNT; ++i){
        print_files(i, d);
    }

    // What field we working with.
    // It's make code simpler.
    struct field *field = &windows[current_window];

    file_count = print_files(current_window, d);
    do {
        c = getch();
        field = &windows[current_window];
        int files_per_region = 0;
        if (file_count < getmaxy(field->w)) {
            files_per_region = file_count;
        } else {
            files_per_region = getmaxy(field->w) - 1;
        }

        // If we have files to only one screen, we need to recalculate files_per_region
        if (file_count == files_per_region) {
            files_per_region--;
        }

        switch (c) {
            case KEY_DOWN :
            case 'j':
                field->current_line = (field->current_line + 1);

                // текущая строка ниже экрана
                if (field->current_line > getmaxy(field->w) - 1) {
                    field->current_line = getmaxy(field->w) - 1;
                    field->current_region++;
                    if (field->current_region > file_count - getmaxy(field->w)) {
                        field->current_region = file_count - getmaxy(field->w);
                        if (field->current_region < 0) {
                            field->current_region = 0;
                        }
                        field->current_line = min(getmaxy(field->w), files_per_region);
                    }
                } else if (field->current_line > files_per_region) {
                    field->current_line = files_per_region;
                }

            break;

            case KEY_UP:
            case 'k':
                field->current_line = (field->current_line - 1);
                if (field->current_line < 0) {
                    field->current_line = 0;
                    field->current_region -= 1;
                    if (field->current_region < 0) {
                        field->current_region = 0;
                    }
                }
            break;

            case KEY_NPAGE:
            case 'd':
                field->current_region += getmaxy(field->w);
                if (field->current_region > file_count - getmaxy(field->w)) {
                    field->current_region = file_count - getmaxy(field->w);
                    if (field->current_region < 0) {
                        field->current_region = 0;
                    }
                    field->current_line = min(getmaxy(field->w), files_per_region);
                }
            break;

            case KEY_PPAGE:
            case 'u':
                field->current_region -= getmaxy(field->w);
                if (field->current_region < 0) {
                    field->current_region = 0;
                    field->current_line = 0;
                }
            break;

            case 10: // ENTER
            case 'l':
                // Is directory selected?
                if (d[field->current_line]->d_type == 4) {
                    strcat(field->current_path, "/");
                    strcat(field->current_path,
                           d[field->current_region + field->current_line]->d_name);
                    clear_path(field->current_path);
                    wrefresh(field->out);
                    field->current_region = 0;
                    field->current_line = 0;
                }
            break;

            // up one dir
            case 'h':
                strcat(field->current_path, "/..");
                clear_path(field->current_path);
                mvwprintw(field->out, getmaxy(field->out) - 1, 1, "%s", field->current_path);
                wrefresh(field->out);
                field->current_region = 0;
                field->current_line = 0;
            break;

            // Yank - COPY FILE
            case 'y':
                {
                    mvwprintw(stdscr, 0, 0, "%d", popup("Tessting popup"));
                    break;

                    char *dest = malloc(strlen(windows[current_window + 1].current_path +
                                strlen(d[field->current_line]->d_name) + 1));
                    dest[0] = '\0';

                    // path from window + '/' + file name
                    strcat(dest, windows[current_window + 1].current_path);
                    strcat(dest, "/");
                    strcat(dest, d[field->current_line]->d_name);

                    chdir(field->current_path);
                    copy(d[field->current_line]->d_name, dest, status);

                    wrefresh(status);
                    refresh();
                    break;
            }

            case 9: // TAB
                current_window = (current_window + 1) % WINDOW_COUNT;
            break;
        }
        file_count = print_files(current_window, d);
    } while(c != 'q');
}
