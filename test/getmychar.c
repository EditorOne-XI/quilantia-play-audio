#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    initscr();
    mvprintw(0, 0, "Enter Key: ");
    int c = getch();
    mvprintw(1, 0, "Key %c (%d)", c, c);
    getch();
    char *path = getenv("PATH");
    if (path) {
        mvprintw(3, 0, "Path getenv: %s", path);
        char *path_copy = strdup(path);
        mvprintw(6, 0, "Path strdup: %s", path_copy);
        char *dir = strtok(path_copy, ":");
        char full_path[1024];
        while (dir != NULL) {
            snprintf(full_path, sizeof(full_path), "%s/%s", dir, "cava");
            if (access(full_path, X_OK) == 0) {
                free(path_copy);
                mvprintw(10, 0, "cava command exists!");
                goto endmain;
            }
            dir = strtok(NULL, ":");
        }
        free(path_copy);
    }
endmain:
    getch();
    getch();
    endwin();
    return 0;
}
