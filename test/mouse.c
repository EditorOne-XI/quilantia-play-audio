#include <ncurses.h>

int main() {
    initscr();
    // cbreak();
    // nodelay(stdscr, TRUE);
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    mousemask(BUTTON1_PRESSED | BUTTON1_CLICKED, NULL);
    mouseinterval(0);
    printw("Touch or click anywhere!\n");
    refresh();

    int c, mx = 0, my = 0;
    MEVENT event;
    while ((c = getch()) != 'q') {
        flushinp();
        if (c == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                if (event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED)) {
                    move(event.y, event.x);
                    mx = event.x;
                    my = event.y;
                    addch('*'); 
                }
            }
        }
        if (c == 32) clear();
        mvprintw(0, 0, "Touch or click anywhere! x = %d, y = %d\n", mx, my);
        refresh();
    }
    endwin();
    return 0;
}
