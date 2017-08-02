#include "print_files.h"

// Clear path, deleting "./" & "../" dirs from it.
// And making path cannonical
void clear_path(char *path)
{
    //char *result2 = (char *)malloc(sizeof(char) * 2048);
    char result[2048];

    result[0] = '\0';

    chdir(path);
    getcwd(result, sizeof(result));
    strcpy(path, result);
}

// Print files to window, using its own current_path, and
// don't store any another data.
int print_files(int window, struct dirent **d)
{
    DIR *dirp = opendir(windows[window].current_path);
    WINDOW *win_to_print = windows[window].w;
    int i = -1;
    int file_count = 0;
    struct dirent *curr_dir;

    // Reading current directory files into array
    wclear(win_to_print);
    while(dirp) {
        if ((curr_dir = readdir(dirp)) != NULL) {
            i++;
            d[i] = curr_dir;
        } else {
            closedir(dirp);
            break;
        }
    }
    file_count = i;

    // Printing only selected region
    for (i = windows[window].current_region; i < getmaxy(windows[window].w) + windows[window].current_region ; ++i){
            if (i == file_count) {
                break;
            }
            if (windows[window].current_line == i - windows[window].current_region) {

                wattron(win_to_print, COLOR_PAIR(2));
                wprintw(win_to_print, "%s\n", d[i]->d_name);
                wattroff(win_to_print, COLOR_PAIR(2));
                continue;
            }
            if (d[i]->d_type == 4) {
                wattron(win_to_print, COLOR_PAIR(3));
                wprintw(win_to_print, "%s\n", d[i]->d_name);
                wattroff(win_to_print, COLOR_PAIR(3));
            } else {
                wprintw(win_to_print, "%s\n", d[i]->d_name);
            }
    }

    mvwprintw(windows[window].out, getmaxy(windows[window].out) - 1, 1,
            "%s\n", windows[window].current_path);

    wrefresh(win_to_print);
    wrefresh(windows[window].out);
    return file_count;
}
