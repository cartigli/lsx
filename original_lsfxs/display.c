#include <ncurses.h>
#include <string.h>

int main(void) {
    initscr(); /* init curses mode */
    noecho();
    cbreak(); /* allows Ctrl-C to kill */
    curs_set(0);  /* hide cursor */
    keypad(stdscr, TRUE); /* enable arrow keys */

    /* get curr. terminal dimensions */
    int yMax, xMax;
    getmaxyx(stdscr, yMax, xMax);

    /* prepare message */
    const char *msg = "Hello, killer!";
    const char *alt = "Hello, world!";
    int msglen = strlen(msg);
    int y = yMax / 2;
    int x = (xMax - msglen) / 2;
    int ay = yMax / 2;
    int alt_msglen = strlen(alt);
    int ax = (xMax - alt_msglen) / 2;

    int ch;
    while (1) {
        /* print msg to user */
        mvprintw(y, x, msg);
        refresh();

        /* get input */
        ch = getch();
        if (ch == 'q') break; /* exit on 'q' press */
        else if (ch == KEY_DOWN) /* or down arrow */
        {
             break;
         }
    }

    /* clean up */
    endwin();
    return 0;
}