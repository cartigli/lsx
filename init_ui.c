#include <ncurses.h>

int main(void)
{
    int ch;

    initscr();                  /* init ncurses mode */
    raw();                      /* line buff disabled */
    keypad(stdscr, TRUE);       /* capture fx keys */
    noecho();                   /* don't echo user input on screen */

    printw("Type any char to see it in bold\n");
    ch = getch();               /* since we called raw(), */
                                /* we the user doesn't have */
                                /* to press enter before we */
                                /* recieve the char typed */
    if (ch == KEY_F(1))
    {
        printw("F1 KEY PRESSED");
    } else {
        printw("The pressed key is ");
        attron(A_BOLD);
        printw("%c", ch);
        attroff(A_BOLD);
    }
    refresh();                  /* print to real screen */
    getch();                    /* wait for user input */
    endwin();                   /* end curses mode */

    return 0;
}

//int omain(void)
//{
//    initscr();              /* initiate curses mode */
//    printw("hello, world"); /* print some text */
//    refresh();              /* print it to the screen */
//    getch();                /* wait for user input */
//    endwin();               /* end curses mode (when input's detected) */

//    return 0;
//}

//void alternates(void)
//{
//    keypad();                   /* enabled the reading of fx keys like f1, f2, down/up arrow */
//    keypad(stdscr, TRUE);       /* to enable this for reg. screens */
//}