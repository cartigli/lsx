#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <ncurses.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fstypes.h"

int read_from(FileSystemNode* ff) {
    if (ff->is_dir) {return 0; }
    char *path = malloc(MAX_FILENAME);
    path[0] = '\0';

    untraverse(ff, path);

    free(path);
    return view_file(path);
}

int view_file(char *path) {
    // FILE *f = fopen(path, "r");
    // if (!f) { return 0; }

    Buffer *b = buffer_load(path);
    // int n_lines = 0;  /* count the lines */
    // int max_line = 0; /* find the longest line */
    // int cur_len = 0;  /* counter */
    // int c;
    // while ((c = fgetc(f)) != EOF) {
    //     if (c == '\n') {
    //         n_lines++;
    //         if (cur_len > max_line) { max_line = cur_len; }
    //         cur_len = 0;
    //     } else { cur_len++; }
    // }
    // /* add line w.no trailing \n */
    // if (cur_len > 0) { n_lines++; }
    // rewind(f); /* reset open file */
    int max_line = 0;
    int n_lines = b->n_lines;
    for (int i = 0; i < n_lines; i++) {
        if (b->lines[i]->len > max_line) { max_line = b->lines[i].len; }
    }

    curs_set(0);

    int screen_h, screen_w;
    getmaxyx(stdscr, screen_h, screen_w);

    /* make pad atleast size of window incase file doesn't fill */
    /* if condition ? expression if true : expression if false; */
    int pad_w = (max_line > screen_w) ? max_line + 1 : screen_w;
    WINDOW *pad = newpad(n_lines + 1, pad_w);
    keypad(pad, TRUE);

    /* read the file into the pad, line by line */
    // char line[4096];
    // int row = 0;
    // while (fgets(line, sizeof(line), f)) {
    //     mvwprintw(pad, row, 0, "%s", line);
    //     row++;
    // }
    // fclose(f);
    int row = 0;
    for (int i = 0; i < n_lines; i++) {
        mvwprintw(pad, row, 0, "%s", b->lines[i].data);
        row++;
    }

    /* scrolling state : which row/column is in the top-left of the viewport */
    int pad_row = 0;
    int pad_col = 0;
    int view_h = screen_h - 1; /* room for status bar at the bottom */
    int view_w = screen_w;

    clear();
    refresh();

    int ch;
    while (1) {
        mvprintw(view_h, 0, "line %d/%d q to quit", pad_row + 1, n_lines);
        clrtoeol();
        refresh();

        prefresh(pad, pad_row, pad_col, 0, 0, view_h - 1, view_w -1);

        ch = wgetch(pad);
        switch(ch) {
            case KEY_DOWN: pad_row++; break;
            case KEY_UP:   pad_row--; break;
            case KEY_RIGHT: pad_col += 8; break;
            case KEY_LEFT: pad_col -= 8; break;
            case 'q': case 27: goto done;
        }

        /* clamp */
        if (pad_row > n_lines - view_h) { pad_row = n_lines - view_h; }
        if (pad_row < 0) { pad_row = 0; }
        if (pad_col > pad_w - view_w) { pad_col = pad_w - view_w; }
        if (pad_col < 0) { pad_col = 0; }
    }
done:
    delwin(pad);
    return 1;
}