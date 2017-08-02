#include "copy.h"

pthread_mutex_t print_window_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  print_window_cond = PTHREAD_COND_INITIALIZER;

struct copy_params {
    char *source;
    char *destination;
    WINDOW *win;
};

struct print_window {
    WINDOW *win_to_print;
    int line_to_print;
    int percent_coplete;
    struct copy_params cp;
};


// Print current percentage progress in allowed window.
void * show_progress(void *param)
{
    struct print_window *pw = (struct print_window *)param;

    pthread_mutex_lock(&print_window_mutex);
    getmaxx(pw->win_to_print);
    do {
        pthread_cond_wait(&print_window_cond, &print_window_mutex);
        unsigned int width = getmaxx(pw->win_to_print);
        unsigned int fill = ((double) pw->percent_coplete / (double)100) * (double)width;

        //We place status message to string by SPRINTF, and then print it  by chars.
        char status[width];

        sprintf(status, "[%.3d]Copying: '%s' to '%s'", pw->percent_coplete, pw->cp.source, pw->cp.destination);
        for (unsigned int i = strlen(status); i < width; ++i){
            status[i] = ' ';
        }
        for (unsigned int i = 0; i < width; ++i){
            if (i < fill) {
                wattron(pw->win_to_print, COLOR_PAIR(2));
                mvwprintw(pw->win_to_print, pw->line_to_print, i, "%c", status[i]);
                wattroff(pw->win_to_print, COLOR_PAIR(2));
            } else {
                mvwprintw(pw->win_to_print, pw->line_to_print, i, "%c", status[i]);
            }
        }

        wrefresh(pw->win_to_print);
        refresh();
    } while(1);
    pthread_mutex_unlock(&print_window_mutex);
}

void * _copy(void *param)
{
    struct copy_params *p = (struct copy_params *) param;

    pthread_t progress;
    struct print_window pw;
    FILE *src, *dst;

    pw.win_to_print = p->win;
    pw.cp = *p;
    pw.line_to_print = 0;
    pthread_create(&progress, NULL, &show_progress, (void*)&pw);

    // FILES OPEN
    src = fopen(p->source, "rb");
    dst = fopen(p->destination, "wb");
    if (src == NULL || dst == NULL) {
        pthread_cancel(progress);
        pthread_exit(NULL);
    }

    // Detect size
    fseek(src, 0, SEEK_END);
    long source_file_size = ftell(src);
    fseek(src, 0, SEEK_SET);

    char buf[1024];
    int readed;
    while(true) {
        readed = fread(buf, sizeof(buf), 1, src);
        if (readed == 0) {
            break;
        }
        fwrite(buf, sizeof(buf), 1, dst);

        pthread_mutex_lock(&print_window_mutex);
        pw.percent_coplete = ((double) ftell(dst) / (double)source_file_size) * 100.0;
        pthread_cond_signal(&print_window_cond);
        pthread_mutex_unlock(&print_window_mutex);
    }

    pthread_mutex_lock(&print_window_mutex);
    pw.percent_coplete = 100;
    pthread_cond_signal(&print_window_cond);
    pthread_mutex_unlock(&print_window_mutex);

    fclose(src);
    fclose(dst);
    pthread_cancel(progress);
    return NULL;

}

void copy(char *source, char *destination, WINDOW *win)
{
    if (strcmp(source, destination) == 0) {
        return;
    }

    pthread_t t;
    pthread_mutex_unlock(&print_window_mutex);
    struct copy_params *p = malloc(sizeof(struct copy_params));
    p->source = malloc(strlen(source) + 1);
    strcpy(p->source, source);
    p->destination = malloc(strlen(destination) + 1);
    strcpy(p->destination, destination);
    p->win = win;
    pthread_create(&t, NULL, _copy, p);
    pthread_join(t, NULL);
}
