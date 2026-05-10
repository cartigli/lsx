#include <ncurses.h>

int main() {
    WINDOW *menu_win;
    int n_choices = 5;
    char *choices[] = {"Option 1", "Option 2", "Option 3", "Option 4", "Exit"};
    int choice = 0;
    int fin = 0;
    int ch;

    initscr();
    cbreak();
    noecho();

    // Create menu window
    menu_win = newwin(10, 30, 5, 5);
    keypad(menu_win, TRUE); /* get the key strokes *from the window* */
    box(menu_win, 0, 0); /* outline the window */
    wrefresh(menu_win); /* refresh the window to show */

    while (!(fin)) { /* Draw choices */
        for (int i = 0; i < n_choices; i++) {
            if (i == choice) {
                /* wattron is declaring the highlighter ON */
                wattron(menu_win, A_REVERSE);
            }
            /* move window print window (                 */
            /* args: *win, int y, int x, const char *fmt) */
            mvwprintw(menu_win, i + 2, 2, "%s", choices[i]);
            if (i == choice) {
                /* wattroff is just declaring the tool as OFF, */
                /* not erasing the previous highlight          */
                wattroff(menu_win, A_REVERSE);
            }
        }
        wrefresh(menu_win);

        ch = wgetch(menu_win);
        switch (ch) {
            case KEY_UP:
                /* if the user presses key up at option 0 */
                /* the choice would go to -1 w.out this */
                if (choice > 0) choice--;
                break; /* if choice > 0, subtract 1 */
                /* but DONT if otherwise, limit  */
            case KEY_DOWN: /* vice versa here */
                if (choice < n_choices - 1) choice++;
                break;
            case 10: /* Return (Line Feed <\n>) */
            case 13: /* Return (Std. Carriage) (case 10 inherits) */
                /* Handle selection */
                // break;
                fin++;
            case 27: // Escape
                goto end;
        }
    }
end:
    endwin();
    if (choice) {
        printf("selection: %s\n", choices[choice]);
    } else { printf("no selection\n"); }
    return 0;
}
