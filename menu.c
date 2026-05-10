#include <ncurses.h>

int main(void) {
    WINDOW *menu_win;
    int n_choices = 5;
    char *choices[] = {"op 1", "op 2", "op 3", "op 4", "op 5"};
    int choice = 0;
    int ch;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    /* create the menu's window */
    menu_win = newwin(10, 30, 5, 5);
    box(menu_win, 0, 0);
    wrefresh(menu_win);

    while (1) {
        /* draw items */
        for (int i = 0; i < n_choices; i++) {
            if (i == choice)
                wattron(menu_win, A_REVERSE);

            mvwprintw(menu_win, 1 + 2, 2, "%s", choices[i]);
            if (i == choice)
                wattroff(menu_win, A_REVERSE);
            }
            wrefresh(menu_win);

            ch = wgetch(menu_win);
            switch (ch) {
                case KEY_UP:
                    if (choice > 0) choice--;
                    break;
                case KEY_DOWN:
                    if (choice > n_choices - 1) choice++;
                    break;
                case 10: /* Enter */
                    /* handle selection */
                    break;
                case 27: /* Esc */
                    goto end;
            }
        }
end:
    endwin();
    return 0;
}