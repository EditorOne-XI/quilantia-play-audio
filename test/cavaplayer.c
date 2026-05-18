#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_PATH "/data/data/com.termux/files/usr/tmp/cava_fifo"
#define CONF_PATH "/data/data/com.termux/files/usr/tmp/cava_config"

void stop_all() {
    system("pkill -9 play-audio 2>/dev/null");
    system("pkill -9 sox 2>/dev/null");
    system("pkill -9 cava 2>/dev/null");
}

void cleanup(int sig) {
    stop_all();
    unlink(FIFO_PATH);
    unlink(CONF_PATH);
    endwin();
    exit(0);
}

int audio_playing() { return system("pgrep play-audio > /dev/null") == 0; }

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    if (access(FIFO_PATH, F_OK) == -1) { mkfifo(FIFO_PATH, 0666); }

    initscr();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0);

    int tlines, tcols;
    getmaxyx(stdscr, tlines, tcols);
    int cava_h = (tlines * 30) / 100;
    FILE *conf = fopen(CONF_PATH, "w");
    if (conf) {
        fprintf(conf, "[input]\n");
        fprintf(conf, "method = fifo\n");
        fprintf(conf, "source = %s\n", FIFO_PATH);

        fprintf(conf, "[output]\n");
        fprintf(conf, "method = noncurses\n");

        fprintf(conf, "[general]\n");
        fprintf(conf, "max_height = 30\n");

        fprintf(conf, "[smoothing]\n");
        fprintf(conf, "monstercat = 1\n");
        fclose(conf);
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "play-audio \"%s\" > /dev/null 2>&1 &", argv[1]);
    system(cmd);
    // sox + faad2 than ffmpeg plan discontinued
    snprintf(cmd, sizeof(cmd),
        "sox -q \"%s\" -t raw -r 44100 -c 2 -b 16 - | pv -q -L 176400 > %s &", argv[1], FIFO_PATH);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "cava -p %s &", CONF_PATH);
    system(cmd);

    int h = 6, w = 48;
    WINDOW *win = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);
    while (audio_playing()) {
        attron(A_INVIS);
        mvprintw(1, 0, " null ");
        attroff(A_INVIS);
        mvprintw(2, 0, "Playing: %s", argv[1]);
        mvprintw(3, 0, "Press 'q' to quit.");

        wclear(win);
        delwin(win);
        if (LINES >= h + 2 || COLS >= w + 2) {
            win = newwin(h, w, (LINES - h - 1) / 2, (COLS - w) / 2);
            wattron(win, COLOR_PAIR(4) | A_BOLD);
            box(win, 0, 0);
            mvwprintw(win, 0, 5, " Loop Music ");
        }
        wnoutrefresh(win);
        wnoutrefresh(stdscr);
        doupdate();

        int c = getch();
        if (c == 'q') { break; }
        if (c == KEY_RESIZE) {
            clear();
            continue;
        }
        usleep(100000);
    }
    cleanup(0);
    return 0;
}
