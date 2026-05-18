/*
 * MIT License
 * 
 *     ██████╗   ██████╗██████╗
 *   ██╔════╗██╗ ██╔═██║██╔═██║
 *   ██║ ██ ║██║ ██████║██████║
 *   ██╚╗▄▄╔╝██║ ██╔═══╝██╔═██║
 *   ╚═██████╔═╝ ██║    ██║ ██║
 *     ╚══▀█▄╝   ╚═╝    ╚═╝ ╚═╝
 * Copyright (c) 2026 EditorOne XI
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "audio_metadata.h"
#include "stringcustom.h"
#include <asm-generic/fcntl.h>
#include <asm-generic/signal.h>
#include <bits/strcasecmp.h>
#include <ctype.h>
#include <getopt.h>
#include <libgen.h>
#include <ncurses.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MIN_WIDTH 52
#define MIN_HEIGHT 18
#define MAX_FILES 1024
#define BUF_SIZE 1024

// the code seems to be awful lol
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "Quilantia Play Audio"
#endif

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "1.0"
#endif

// cava custom filepaths
#ifndef FIFO_PATH
#define FIFO_PATH "/tmp/qpa_cava_fifo"
#endif

#ifndef CONF_PATH
#define CONF_PATH "/tmp/qpa_cava_config"
#endif

#define PKILLPA "pkill -n play-audio >/dev/null 2>&1"
#define MOUSEREG if (doMouse) mousemask(BUTTON1_PRESSED | BUTTON1_CLICKED | BUTTON1_RELEASED, NULL)

static char NOTIF_ID[16] = "NOTIF-QPA";
static char cmdname[32] = "[!]";
static char queue[BUF_SIZE] = " None ";
static pid_t currentpid;
static pid_t playaudio_pid = -1;
static pid_t fifo_pid = -1;
static pid_t cava_pid = -1;
static FILE *fp;
static int use_cava = 0, cavafocus = 0;
static int doMouse = 0;

static char *PKGNAME_FIRST() {
    char first[16];
    int i = 0;
    while (PACKAGE_NAME[i] != '\0' && i < 15) {
        if (isspace((unsigned char)PACKAGE_NAME[i])) break;
        first[i] = PACKAGE_NAME[i];
        i++;
    }
    if (i == 0) return strdup("Quilantia");
    first[i] = '\0';
    return strdup(first);
}

static void PLAYAUDIO_KILL() {
    if (playaudio_pid > 0)
        kill(playaudio_pid, SIGTERM);
    else
        system(PKILLPA);
}

static void popup_error(const char *msg) {
    initscr();
    int h = 9;
    int w = intmin(strlen(msg) + 6, COLS - 6);

    WINDOW *err_win = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);
    WINDOW *err_wincont = derwin(err_win, h - 4, w - 9, 2, 4);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_RED, -1);
    clear();
    wattron(err_win, COLOR_PAIR(1) | A_BOLD);
    wattron(err_wincont, COLOR_PAIR(1) | A_BOLD);
    box(err_win, 0, 0);
    mvwprintw(err_win, 0, 2, " ERROR ");
    mvwprintw(err_wincont, 0, 0, "%.*s", (w - 9) * 3, msg);
    wattroff(err_wincont, COLOR_PAIR(1) | A_BOLD);
    wattroff(err_win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(err_wincont, h - 5, 0, "Press any key to exit...");
    wrefresh(err_win);

    wgetch(err_wincont);
    delwin(err_wincont);
    delwin(err_win);
    endwin();
    if (fp) pclose(fp);
    system(PKILLPA);
    exit(1);
}

// clang-format off
static const char *menukeys[18][2] = {
    {"up, k", "move cursor up"},
    {"down, j", "move cursor down"},
    {"left, h", "move cursor left"},
    {"right, l", "move cursor right"},
    {"i", "show audio info"},
    {"space, p", "play music"},
    {"s", "stop music"},
    {"g", "(grep) filter files"},
    {"m", "toggle mouse/screen touch"},
    {"r", "toggle recursive read"},
    {"v", "toggle files sort"},
    {"w, W", "loop mode"},
    {"y, Y", "custom queue mode"},
    {"C", "toggle cava for loop mode"},
    {"D", "change directory"},
    {"R", "reload files"},
    {"X", "delete highlighted file"},
    {"q, Q", "exit"}
};
static const size_t menusc = sizeof(menukeys) / sizeof(menukeys[0]);

static const char *features = ""
/*****************************************/
"[Loop Mode]\n"
"- Common Music Player feature to roll\n"
"  playlist.\n"
"- Music cycles when the queue has\n"
"  reached the end of playlist.\n"
"* Keys 'k' and 'j' changed for previous\n"
"  music if available and next music.\n"
"\n"
"[Cava in Loop Mode]\n"
"- Included from intallation to add audio\n"
"  visual inside Loop Mode.\n"
"  (see '%1$s --status' for more)\n"
"\n"
"[Custom Queue Mode]\n"
"- Common Music Player feature to set\n"
"  specific music to play in select\n"
"  order.\n"
"* Keys 'p' and 's' changed to add music\n"
"  and remove music to the custom queue.\n"
"* Play custom queue with 'w' key\n"
"  entering Loop Mode.\n"
"\n"
"[Change Directory]\n"
"- Move current working directory through\n"
"  user input. Environment Shell\n"
"  variables work except for ~ variable.\n"
"\n"
"[Termux: Notification]\n"
"- Termux:API notification for %1$s\n"
"* Toggle notification with 'N' key.";
/*****************************************/
static const char *about = ""
"             ██████╗   ██████╗██████╗\n"
"           ██╔════╗██╗ ██╔═██║██╔═██║\n"
"           ██║ ██ ║██║ ██████║██████║\n"
"           ██╚╗▄▄╔╝██║ ██╔═══╝██╔═██║\n"
"           ╚═██████╔═╝ ██║    ██║ ██║\n"
"             ╚══▀█▄╝   ╚═╝    ╚═╝ ╚═╝\n"
"            Quilantia    Play-Audio\n"
"\n"
"         Written by EditorOne XI. v%s\n"
"   Simple Playlist UI wrapper for play-audio.";
// clang-format on

static void help_window() {
    int h = 14, w = 50;
    WINDOW *help_win = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);
    WINDOW *help_derwin = derwin(help_win, h - 2, w - 2, 1, 1);
    WINDOW *help_pad = NULL;
    char *uhname = PKGNAME_FIRST();
    int fnlen = strlen(uhname);
    int featuresc = strlflen(features) + 1;
    int c;
    int page = 0;
    int phelpsc = 0, phelp_maxsc = 0;
    int helpby, helpbx;
    getbegyx(help_win, helpby, helpbx);

    while (1) {
        werase(help_win);
        if (help_pad != NULL) {
            delwin(help_pad);
            help_pad = NULL;
        }
        wattron(help_win, COLOR_PAIR(3) | A_BOLD);
        box(help_win, 0, 0);
        mvwprintw(help_win, 0, 2, " %s | ", uhname);
        if (page == 0) wattrset(help_win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
        mvwprintw(help_win, 0, 6 + fnlen, "Keys");
        wattrset(help_win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(help_win, 0, 10 + fnlen, " | ");
        if (page == 1) wattrset(help_win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
        mvwprintw(help_win, 0, 13 + fnlen, "Features");
        wattrset(help_win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(help_win, 0, 21 + fnlen, " | ");
        if (page == 2) wattrset(help_win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
        mvwprintw(help_win, 0, 24 + fnlen, "About");
        wattrset(help_win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(help_win, 0, 29 + fnlen, " ");
        wattroff(help_win, COLOR_PAIR(3) | A_BOLD);
        switch (page) {
        case 0:
            phelp_maxsc = menusc;
            help_pad = newpad(phelp_maxsc, w - 9);
            wattron(help_derwin, COLOR_PAIR(3) | A_BOLD);
            wattrset(help_pad, COLOR_PAIR(7) | A_BOLD);
            for (int k = 0; k < (int)menusc; k++) {
                mvwprintw(help_pad, k, 0, "%10s   %-.27s", menukeys[k][0], menukeys[k][1]);
            }
            if (phelpsc < phelp_maxsc - 9)
                mvwprintw(help_pad, phelpsc + 9, 0, "%1$14s--- more ---%1$15s", "");
            wattroff(help_pad, COLOR_PAIR(7) | A_BOLD);
            break;
        case 1:
            phelp_maxsc = featuresc;
            help_pad = newpad(phelp_maxsc, w - 9);
            wattrset(help_pad, COLOR_PAIR(7) | A_BOLD);
            wprintw(help_pad, features, cmdname);
            if (phelpsc < phelp_maxsc - 9)
                mvwprintw(help_pad, phelpsc + 9, 0, "%1$14s--- more ---%1$15s", "");
            wattroff(help_pad, COLOR_PAIR(7) | A_BOLD);
            break;
        case 2:
            wattron(help_derwin, COLOR_PAIR(7) | A_BOLD);
            mvwprintw(help_derwin, 1, 0, about, PACKAGE_VERSION);
            break;
        default:
            page = 0;
            break;
        }
        char *closetxt = " Press 'q' to close help. ";
        wattron(help_win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(help_win, h - 1, w - strlen(closetxt) - 2, "%s", closetxt);
        wattroff(help_win, COLOR_PAIR(3) | A_BOLD);
        wnoutrefresh(help_win);
        if (page == 0 || page == 1)
            pnoutrefresh(help_pad, phelpsc, 0, helpby + 2, helpbx + 5, helpby + 11, helpbx + 45);
        doupdate();

        c = getch();
        switch (c) {
        case 'h':
        case KEY_LEFT:
            if (page == 0) break;
            phelpsc = 0;
            page--;
            break;
        case 'l':
        case KEY_RIGHT:
            if (page == 2) break;
            phelpsc = 0;
            page++;
            break;
        case 'j':
        case KEY_DOWN:
            phelpsc = (phelpsc + 10 > phelp_maxsc ? phelp_maxsc - 9 : phelpsc + 1);
            break;
        case 'k':
        case KEY_UP:
            phelpsc = (phelpsc - 1 < 0 ? 0 : phelpsc - 1);
            break;
        case KEY_RESIZE:
        case 10:
        case 'Q':
        case 'q':
            goto endhelp;
        }
    }
endhelp:
    free(uhname);
    delwin(help_pad);
    delwin(help_derwin);
    delwin(help_win);
    refresh();
}

static void metadata_window(const char *filename) {
    int h = 9, w = COLS - 6;
    WINDOW *md_win = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);
    WINDOW *md_wincont = derwin(md_win, h - 4, w - 8, 2, 4);
    init_pair(6, COLOR_GREEN, -1);

    char *empty[4];
    char **metadata = get_metadata_universal(filename);
    char *duration = get_duration_universal(filename);
    if (metadata == NULL) {
        metadata = empty;
        memset(metadata, 0, 4 * sizeof(char *));
    }
    char *closetxt = " Press 'i' to close window. ";
    int strlim = COLS - 20;
    int c;
    int page = 0;
    while (1) {
        flushinp();
        werase(md_win);
        wattron(md_win, COLOR_PAIR(6) | A_BOLD);
        box(md_win, 0, 0);
        mvwprintw(md_win, 0, 2, " Metadata | File ");
        wattrset(md_win, COLOR_PAIR(6) | A_BOLD);
        if (page == 1) {
            mvwprintw(md_win, 0, 19, "| ");
            wattrset(md_win, COLOR_PAIR(6) | A_BOLD | A_REVERSE);
            mvwprintw(md_win, 0, 21, "CC");
            wattrset(md_win, COLOR_PAIR(6) | A_BOLD);
            mvwprintw(md_win, 0, 23, " ");
        }
        mvwprintw(md_win, h - 1, w - strlen(closetxt) - 2, "%s", closetxt);
        wattroff(md_win, COLOR_PAIR(6) | A_BOLD);
        switch (page) {
        case 0:
            wattron(md_wincont, COLOR_PAIR(7) | A_BOLD);
            // clang-format off
            if (metadata != empty) {
                mvwprintw(md_wincont, 0, 0, "Title:    %.*s", strlim, strisnull(metadata[0], NOTSTR));
                mvwprintw(md_wincont, 1, 0, "Artist:   %.*s", strlim, strisnull(metadata[1], NOTSTR));
                mvwprintw(md_wincont, 2, 0, "Album:    %.*s", strlim, strisnull(metadata[2], NOTSTR));
                mvwprintw(md_wincont, 3, 0, "Year:     %.*s", strlim, strisnull(metadata[3], NOTSTR));
            } else
                mvwprintw(md_wincont, 0, 0, "Not found.");
            mvwprintw(md_wincont, 4, 0, "Duration: %.*s", strlim, strisnull(duration, "--/--"));
            // clang-format on
            wattroff(md_wincont, COLOR_PAIR(7) | A_BOLD);
            break;
        case 1:
            wattron(md_wincont, COLOR_PAIR(7) | A_BOLD);
            mvwprintw(md_wincont, 0, 0, "--- File Metadata Binary Reader ---");
            mvwprintw(md_wincont, 1, 0, "* Binary Reader generated by Gemini 3 Flash");
            mvwprintw(md_wincont, 2, 0, "* Modified and implemented by EditorOne XI.");
            mvwprintw(md_wincont, 3, 0, "Accepting File Formats:");
            mvwprintw(md_wincont, 4, 0, "+[flac,m4a,mp3,ogg,opus,wav] -[3gp,aac]");
            wattroff(md_wincont, COLOR_PAIR(7) | A_BOLD);
            break;
        }
        wrefresh(md_win);
        c = getch();
        c = tolower(c);
        switch (c) {
        case 'h':
        case KEY_LEFT:
            if (page == 0) break;
            page--;
            break;
        case 'l':
        case KEY_RIGHT:
            if (page == 1) break;
            page++;
            break;
        case KEY_RESIZE:
        case 10:
        case 'Q':
        case 'q':
        case 'i':
            goto endmdwin;
        }
    }
endmdwin:
    if (metadata != empty || metadata != NULL) {
        for (int i = 0; i < 4; i++)
            free(metadata[i]);
        free(metadata);
    }
    if (duration != NULL) free(duration);
    delwin(md_wincont);
    delwin(md_win);
    refresh();
}

static void playaudio_fork(char *item) {
    if (playaudio_pid > 0) PLAYAUDIO_KILL();
    playaudio_pid = fork();
    if (playaudio_pid < 0) return;
    if (playaudio_pid == 0) {
        int devnull = open("/dev/null", O_RDWR);
        if (devnull != -1) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        char *args[] = {"play-audio", item, NULL};
        execvp(args[0], args);
        playaudio_pid = -1;
        strcpy(queue, " Unable to run play-audio ");
        _exit(1);
    }
}

static void fifo_fork(char *item) {
    if (!use_cava) return;
    fifo_pid = fork();
    if (fifo_pid < 0) return;
    if (fifo_pid == 0) {
        int devnull = open("/dev/null", O_RDWR);
        if (devnull != -1) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        char cmd[4096];
        if (strcmdv("ffmpeg"))
            snprintf(cmd, sizeof(cmd), "(head -c 17640 /dev/zero; ffmpeg -loglevel quiet -i \"%s\" -f s16le -ar 44100 -ac 2 -) | pv -q -L 176400 > %s", item, FIFO_PATH);
        else
            snprintf(cmd, sizeof(cmd), "(head -c 17640 /dev/zero; sox -q \"%s\" -t raw -r 44100 -c 2 -b 16 - 2>/dev/null) | pv -q -L 176400 > %s", item, FIFO_PATH);
        char *args[] = {"sh", "-c", cmd, NULL};
        execvp(args[0], args);
        use_cava = 0;
        fifo_pid = -1;
        _exit(1);
    }
}

static void cava_fork() {
    if (!use_cava) return;
    cava_pid = fork();
    if (cava_pid < 0) return;
    if (cava_pid == 0) {
        int devnull = open("/dev/null", O_RDWR);
        if (devnull != -1) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        char *args[] = {"cava", "-p", CONF_PATH, NULL};
        execvp(args[0], args);
        use_cava = 0;
        cava_pid = -1;
        _exit(1);
    }
}

static int use_cava_check() {
    int retval = 1;
    retval &= (strcmdv("ffmpeg") || strcmdv("sox"));
    retval &= strcmdv("pv");
    retval &= strcmdv("cava");
    return retval;
}

static void cleanup_loopitem() {
    if (playaudio_pid > 0) kill(playaudio_pid, SIGTERM);
    if (fifo_pid > 0) kill(fifo_pid, SIGTERM);
    if (cava_pid > 0) kill(cava_pid, SIGKILL);
    playaudio_pid = -1;
    fifo_pid = -1;
    cava_pid = -1;
    unlink(FIFO_PATH);
    unlink(CONF_PATH);
    clear();
}

static int audio_playing() {
    if (playaudio_pid <= 0) return 0;
    return kill(playaudio_pid, 0) == 0;
}

static int loopitem_window(WINDOW *win, char *item, int isShuf) {
    if (doMouse) mousemask(0, NULL);
    int c = 0;
    timeout(200);
    c = getch();
    if (c == 'j' || c == 'k' || c == 32) goto enditem;
    // skip initialize
    char display[BUF_SIZE];
    char temp[BUF_SIZE] = "";
    int h = 6, w = 48, lc = 0;
    int maxstrw = 44;
    clear();

playagain:
    if (!use_cava_check()) use_cava = 0;
    if (use_cava) {
        if (access(FIFO_PATH, F_OK) == -1) { mkfifo(FIFO_PATH, 0666); }
        FILE *conf = fopen(CONF_PATH, "w");
        // clang-format off
        char *palletes[] = {
            "#ffff00",
            "#ff0000",
            "#0000ff",
            "#ff00ff",
            "#00ffff",
            "#0000ff"
        };
        int rn = rand() % 5;
        if (conf) {
fprintf(conf, ""
"[input]\n"
"method = fifo\n"
"source = %s\n"
"[output]\n"
"method = noncurses\n"
"show_idle_bar_heads = 0\n"
"channels = stereo\n"
"[general]\n"
"max_height = 30\n"
"sensitivity = 100\n"
"[smoothing]\n"
// "monstercat = 0\n" // optional
"noise_reduction = 50\n"
"[eq]\n"
"1=1.0\n2=1.0\n3=1.1\n4=1.2\n5=1.3\n"
"[color]\n"
"horizontal_gradient = 1\n"
"horizontal_gradient_color_1 = '%s'\n"
"horizontal_gradient_color_2 = '%s'\n",
FIFO_PATH, palletes[rn] , palletes[rn + 1]);
            fclose(conf);
        }
        fifo_fork(item);
        cava_fork();
        if (!use_cava) cleanup_loopitem();
        // clang-format on
    }
    playaudio_fork(item);
    if (playaudio_pid < 0) {
        c = 's';
        goto enditem;
    }
    usleep(100);
    timeout(200);
    win = newwin(h, w, (LINES - h - 1) / 2, (COLS - w) / 2);
    wtimeout(win, 200);
    while (audio_playing()) {
        if (use_cava && lc == KEY_RESIZE) kill(cava_pid, SIGCONT);        
        if (cavafocus) goto smallterm;
        if (COLS < w + 4 || LINES < (h * 2) + 2) {
            if (use_cava) goto smallterm;
            switch (isShuf) {
            case 1:
                strcpy(temp, "Current Playlist: ");
                break;
            case 2:
                strcpy(temp, "Custom Queue: ");
                break;
            default:
                strcpy(temp, "");
            }
            attron(COLOR_PAIR(4) | A_BOLD);
            mvprintw(0, 0, "[Loop Mode] %s%s; [k] Prev, [j] Next, [s] Stop Loop%-512s", temp, item, ".");
            attroff(COLOR_PAIR(4) | A_BOLD);
            refresh();
            goto smallterm;
        }
        werase(win);

        wattron(win, COLOR_PAIR(4) | A_BOLD);
        box(win, 0, 0);
        mvwprintw(win, 0, 5, " Loop Music ");

        switch (isShuf) {
        case 1:
            snprintf(temp, sizeof(temp), "Current Playlist: %s", basename(item));
            break;
        case 2:
            snprintf(temp, sizeof(temp), "Custom Queue: %s", basename(item));
            break;
        default:
            snprintf(temp, sizeof(temp), "%s", basename(item));
        }
        snprintf(display, sizeof(display), "%*s%.*s",
            (int)((maxstrw - intmin(strlen(temp), maxstrw)) / 2), "",
            ((int)strlen(temp) > maxstrw ? maxstrw - 3 : maxstrw), temp);
        if ((int)strlen(temp) > maxstrw) strcat(display, "...");
        mvwprintw(win, 2, 2, "%s", display);
        wattrset(win, COLOR_PAIR(5) | A_BOLD);
        mvwprintw(win, 3, (isShuf ? 7 : 12), "%s[j] Next  [s] Stop Loop", (isShuf ? "[k] Prev  " : ""));
        wattroff(win, COLOR_PAIR(5) | A_BOLD);
        wnoutrefresh(stdscr);
        wnoutrefresh(win);
        if (!use_cava) doupdate();

    smallterm:
        if (win == NULL)
            c = getch();
        else
            c = wgetch(win);
        if (lc != c) lc = c;
        switch (c) {
        case KEY_MOUSE: break;
        case KEY_RESIZE:
            if (use_cava) kill(cava_pid, SIGSTOP);
            delwin(win);
            win = newwin(h, w, (LINES - h - 1) / 2, (COLS - w) / 2);
            wtimeout(win, 200);
            flushinp();
            clear();
            continue;
        case 'G':
        case 'g':
            cavafocus = 0;
            clear();
            flushinp();
            continue;
        case 'V':
        case 'v':
            if (!use_cava) break;
            cavafocus = !cavafocus;
            if (cavafocus) {
                kill(cava_pid, SIGSTOP);
                werase(win);
                delwin(win);
                kill(cava_pid, SIGCONT);
                refresh();
            }
            continue;
        case 'k':
            if (!isShuf) break;
            goto enditem;
        case 10:
        case 'p':
            cleanup_loopitem();
            while ((c = getch()) == 'p' || c == 10) {}
            lc = c = 0;
            flushinp();
            goto playagain;
        case 32:
        case 'j':
            goto enditem;
        case 'Q':
        case 'q':
        case 's':
            c = 's';
            timeout(-1);
            MOUSEREG;
            goto enditem;
        }
        if (use_cava) doupdate();
        usleep(1000);
    }
enditem:
    if (fp) pclose(fp);
    cleanup_loopitem();
    delwin(win);
    if (c == 's') return 33280;
    if (c == 'k') return 1;
    return 0;
}

static void loop_window(char *current, int *currentc, char *list[], int listc) {
    int h = 7, w = 28, c = 0;
    WINDOW *loop_win = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);

    wattron(loop_win, COLOR_PAIR(2) | A_BOLD);
    box(loop_win, 0, 0);
    mvwprintw(loop_win, 0, 2, " Loop Mode ^o^ ");
    mvwprintw(loop_win, 2, 3, "[1] Loop Current Music");
    mvwprintw(loop_win, 3, 3, "[2] Loop Playlist");
    mvwprintw(loop_win, 4, 3, "[3] Exit");
    wattroff(loop_win, COLOR_PAIR(2) | A_BOLD);
    wrefresh(loop_win);
    while (1) {
        c = wgetch(loop_win);
        switch (c) {
        case 'Q':
        case 'q':
        case KEY_RESIZE:
        case '3':
            goto exitloop;
        case '1':
        case '2':
            break;
        default:
            continue;
        }
        break;
    }
    PLAYAUDIO_KILL();
    int sysretval;
    if (c == '1') {
        while (1) {
            sysretval = loopitem_window(loop_win, current, 0);
            if (sysretval == 33280) goto exitloop;
        }
    } else {
        int q = *currentc;
        while (1) {
            sysretval = loopitem_window(loop_win, list[q], 1);
            if (sysretval == 33280) {
                *currentc = q;
                goto exitloop;
            }
            if (sysretval == 1) {
                q = (q == 0) ? listc - 1 : q - 1;
            } else {
                q = (q == listc - 1) ? 0 : q + 1;
            }
        }
    }
exitloop:
    delwin(loop_win);
    strcpy(queue, " Exited Loop Mode ");
    refresh();
}

static int deleteFileConfirm = 1;
static int deleteFileWin(char *file) {
    char absfpath[BUF_SIZE];
    char *pptr;
    if (!deleteFileConfirm) goto deleteaction;
    int h = 8, w = 50;
    int lr = 2;
    int c = 0;
    int fnlen = strlen(basename(file));
    char filename[50];
    if (fnlen > 44) {
        snprintf(filename, sizeof(filename), "%-.*s", 41, basename(file));
        strncat(filename, "...", sizeof(filename) - strlen(filename) - 1);
    } else {
        snprintf(filename, sizeof(filename), "%-.*s", 44, basename(file));
    }
    WINDOW *delfile_win = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);
    while (1) {
        wattron(delfile_win, COLOR_PAIR(1) | A_BOLD);
        box(delfile_win, 0, 0);
        mvwprintw(delfile_win, 0, 4, " Removing File from Disk ");
        wattrset(delfile_win, COLOR_PAIR(7) | A_BOLD);
        mvwprintw(delfile_win, 2, 2, "Are you sure you want to delete this file?");
        mvwprintw(delfile_win, 3, 2, "> %-.44s", filename);
        if (lr == 1) wattron(delfile_win, A_REVERSE);
        mvwprintw(delfile_win, 5, (w / 4) - 2, " Yes ");
        wattroff(delfile_win, A_REVERSE);
        if (lr == 2) wattron(delfile_win, A_REVERSE);
        mvwprintw(delfile_win, 5, (w / 4) * 2 - 2, " No ");
        wattroff(delfile_win, A_REVERSE);
        if (lr == 3) wattron(delfile_win, A_REVERSE);
        mvwprintw(delfile_win, 5, (w / 4) * 3 - 5, " Don't ask ");
        wattroff(delfile_win, COLOR_PAIR(7) | A_REVERSE | A_BOLD);
        wnoutrefresh(stdscr);
        wnoutrefresh(delfile_win);
        doupdate();

        c = getch();
        switch (c) {
        case 'h':
        case KEY_LEFT:
            if (lr > 1) lr--;
            break;
        case 'l':
        case KEY_RIGHT:
            if (lr < 3) lr++;
            break;
        case 10:
            if (lr == 2)
                return 0;
            if (lr == 3)
                deleteFileConfirm = !deleteFileConfirm;
        deleteaction:
            pptr = realpath(file, absfpath);
            if (pptr && remove(absfpath) == 0) {
                snprintf(queue, sizeof(queue), " Deleted %s ", basename(file));
                return 1;
            } else {
                strcpy(queue, " Failed to delete file. ");
                return 0;
            }
            return 1;
        case KEY_RESIZE:
        case 'Q':
        case 'q':
            delwin(delfile_win);
            refresh();
            return 0;
        }
    }
}

static void sigexit(int sig) {
    clear();
    endwin();
    printf("[!] Exiting Quilantia Music Player...\n");
    cleanup_loopitem();
    char cleanup_cmd[64];
    if (strcmdv("termux-notification") && strcmp(NOTIF_ID, "NOTIF-QPA")) snprintf(cleanup_cmd, sizeof(cleanup_cmd), "termux-notification-remove %s &", NOTIF_ID);
    system(cleanup_cmd);
    if (fp) pclose(fp);
    kill(currentpid, sig);
    exit(0);
}

static void resetread(char **files, int *count, int *scroll_pos, int *highlight, const char *msg) {
    memset(files, 0, sizeof(char *));
    *count = 0;
    *scroll_pos = 0;
    *highlight = 0;
    if (msg != NULL) strncpy(queue, msg, sizeof(queue));
    clear();
}

int main(int argc, char *argv[]) {
    char *env = getenv("TERM");
    strcpy(cmdname, basename(argv[0]));
    if (env == NULL) {
        printf("%1$s: Unsupported: TERM not specified.\n", cmdname);
        return 1;
    }
    // clang-format off
    int main_retval = 0;
    char *cmddpdc[10] = {
        "play-audio",
        "find", "grep", "sort", "shuf",
        "sed", "head",
        "pgrep", "kill", "pkill"
    };
    for (int i = 0; i < (int)(sizeof(cmddpdc) / sizeof(char*)); i++) {
        if (strcmdv(cmddpdc[i])) continue;
        fprintf(stderr, "%1$s: Dependency: %2$s not found.\n", cmdname, cmddpdc[i]);
        main_retval = 1;
    }
    if (main_retval != 0) return main_retval;

    int opt, isRecursive = 0, rcsv = 0;
    char buffer[BUF_SIZE] = "";
    char grepfiles[BUF_SIZE] = "";
    char sortfiles[9] = "shuf";
    char directory[BUF_SIZE] = "";
    struct option longopts[9] = {
        {"use-cava", no_argument, NULL, 256},
        {"grep", required_argument, NULL, 'g'},
        {"help", no_argument, NULL, 'h'},
        {"mouse", no_argument, NULL, 'm'},
        {"recursive", optional_argument, NULL, 'r'},
        {"sort", required_argument, NULL, 's'},
        {"status", no_argument, NULL, 257},
        {"version", no_argument, NULL, 'v'},
        {0, 0, 0, 0}
    };
    while ((opt = getopt_long(argc, argv, "hg:mr::s:v", longopts, NULL)) != -1) {
        switch (opt) {
        case 256:
            if (!use_cava_check()) {
                fprintf(stderr, "%s: Dependency: Install ffmpeg/sox, pv, and cava to use this feature.\n", cmdname);
                return 1;
            }
            use_cava = 1;
            break;
        case 257:
            if (strcmdv("ffmpeg"))
                strcpy(buffer, "ffmpeg");
            else if (strcmdv("sox"))
                strcpy(buffer, "sox");
            else
                strcpy(buffer, "(null)");
            printf("[Cava for Loop Mode] - %s\n"
                "Dependency = %s\n"
                "FIFO_PATH = %s\n"
                "CONF_PATH = %s\n",
                use_cava_check() ? "Available" : "OFF",
                buffer, FIFO_PATH, CONF_PATH);
            return 0;
        case 'g':
            strcmdsecure(optarg);
            if (strisempty(optarg)) {
                fprintf(stderr, "%s: Error: Argument for grep is empty.\n", cmdname);
                return 1;
            }
            snprintf(grepfiles, sizeof(grepfiles), "| grep %s", optarg);
            break;
        case 'm':
            doMouse = 1;
            break;
        case 'r':
            if (optarg != NULL && atoi(optarg) > 0)
                rcsv = atoi(optarg) + 1;
            isRecursive = 1;
            printf("Reading files recursively...\n");
            break;
        case 's':
            strcpy(sortfiles, "sort -V");
            if (!strcasecmp(optarg, "D"))
                strncat(sortfiles, "r", sizeof(sortfiles) - strlen(sortfiles) - 1);
            break;
        case 'v':
            printf("%s v%s\n", PACKAGE_NAME, PACKAGE_VERSION);
            return 0;
        case 'h': printf(
"%1$s: Usage: %1$s [OPTIONS] [directory]\n"
"Read current directory if not specified.\n"
"An error window shows when no accepted audio files found, error also shows when changing directory from menu.\n"
"\n"
"OPTIONS:\n"
"    -h, --help                  verbose this help and exit\n"
"        --status                verbose %1$s install status and exit\n"
"    -v, --version               verbose %1$s version and exit\n"
"    -g, --grep <syntax>         grep command to filter files\n"
"    -m, --mouse                 enable mouse/screen touch\n"
"    -r[N], --recursive[=N]      read files recursively (default N=0)\n"
"    -s, --sort <A|D>            sort alphabetically (A=ascending, D=descending)\n"
"        --use-cava              use cava for loop mode\n",
cmdname);
            return 2;
        case '?':
            return 1;
        }
    }
    snprintf(buffer, sizeof(buffer), "pgrep -x %s | grep -v %d >/dev/null 2>&1", cmdname, getpid());
    if (system(buffer) == 0) {
        fprintf(stderr, "There can be only one instance of %s process. Sorry!\n", cmdname);
        return 1;
    }
    if (argv[optind] != NULL && chdir(argv[optind]) != 0) {
        fprintf(stderr, "%s: Error: %s not a directory.\n", cmdname, argv[optind]);
        return 1;
    }
    char cmd[4096];
    char input[256];
readagain:
    if (isRecursive && rcsv > 0)
        snprintf(buffer, sizeof(buffer), "-maxdepth %d", rcsv); else strcpy(buffer, "");
    snprintf(cmd, sizeof(cmd),
        "find . %s -regextype posix-egrep -regex \".*\\.(3gp|aac|flac|m4a|mp3|ogg|opus|wav)$\" 2>/dev/null %s | %s | sed 's/[\"$]/\\\\&/g' | head -n %d",
        (isRecursive ? buffer : "-maxdepth 1"), 
        grepfiles, sortfiles, MAX_FILES);
    // clang-format on

    fp = popen(cmd, "r");
    if (fp == NULL) {
        pclose(fp);
        popup_error("Unable to search files.");
    }
    static char *files[MAX_FILES];
    static int count = 0;
    while (fp && fgets(buffer, sizeof(buffer), fp) && count < MAX_FILES) {
        buffer[strcspn(buffer, "\n")] = 0;
        files[count++] = strdup(buffer);
    }
    if (fp) pclose(fp);
    if (count == 0) {
        getcwd(cmd, sizeof(cmd));
        snprintf(buffer, sizeof(buffer),
            "No audio files found in %s. Accepting "
            "*.{3gp,aac,flac,m4a,mp3,ogg,opus,wav}",
            (argc > 1 ? cmd : "this directory"));
        popup_error(buffer);
    }
    static int isQueueMode = 0;
    static VectorMap playlistQueue = {};
    if (isQueueMode != 0) {
        clearVectorMap(&playlistQueue);
        initVectorMap(&playlistQueue, 1);
        strncat(queue, "[Queue Reset]", sizeof(queue) - strlen(queue) - 1);
    }

    initscr();
    start_color();
    use_default_colors();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    MOUSEREG;
    mouseinterval(0);
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_YELLOW, -1);
    init_pair(3, COLOR_CYAN, -1);
    init_pair(4, COLOR_MAGENTA, -1);
    init_pair(5, COLOR_BLUE, -1);
    init_pair(7, COLOR_WHITE, -1);
    static WINDOW *playlist_pad;
    static MEVENT interact;
    currentpid = getpid();
    //
    signal(SIGCHLD, SIG_IGN);
    signal(SIGKILL, sigexit);
    signal(SIGTERM, sigexit);
    signal(SIGINT, sigexit);
    signal(SIGTSTP, sigexit);
    signal(SIGHUP, sigexit);
    //
    getcwd(directory, sizeof(directory));
    snprintf(buffer, sizeof(buffer), " Playlist: %-.*s", COLS - 13, basename(directory));
    if (COLS < MIN_WIDTH || LINES < MIN_HEIGHT) goto keyreturn;
    char header[BUF_SIZE];
    snprintf(header, sizeof(header), " %s | press ? for help ", PACKAGE_NAME);
    srand(time(NULL));

refreshmain:
    playlist_pad = newpad(count, COLS);
    for (int i = 0; i < count; i++) {
        if (!strlen(files[i])) {
            count--;
            i--;
            continue;
        }
        mvwprintw(playlist_pad, i, 0, " %4d. %-*s", i + 1, COLS - 7, basename(files[i]));
    }
    static int loopretval, qm = 0;
    static WINDOW *queue_win;
    static int c = 0;
    static int scroll_pos = 0;
    static int highlight = 0, hlinteract = 0, nullint = 0;
    int width, height, list_height;
    flushinp();
    while (1) {
        getmaxyx(stdscr, height, width);
        attron(A_REVERSE);
        mvprintw(0, 0, "%-*s", COLS, header);
        attroff(A_REVERSE);
        mvprintw(1, 0, "%-*s", COLS, buffer);
        for (int i = 0; i < count; i++) {
            mvwchgat(playlist_pad, i, 0, -1, A_NORMAL, 0, NULL);
        }
        mvwchgat(playlist_pad, highlight, 0, -1, A_REVERSE, 2, NULL);
        if (isQueueMode) {
            for (int i = scroll_pos; i < scroll_pos + list_height; i++) {
                if (hasVectorMapKey(&playlistQueue, i))
                    mvwchgat(
                        playlist_pad, i, 0, -1, (i == highlight ? A_REVERSE : A_NORMAL), 3, NULL);
            }
        }
        attron(A_REVERSE);
        mvprintw(LINES - 1, 0, "%-*s", COLS, queue);
        attroff(A_REVERSE);
        wnoutrefresh(stdscr);
        pnoutrefresh(playlist_pad, scroll_pos, 0, 2, 0, LINES - 2, COLS - 1);
        doupdate();

        if (c != 0) c = 0;
        c = getch();
        list_height = height - 3;
        switch (c) {
        case KEY_RESIZE:
            goto keyreturn;
        case 'k':
        case KEY_UP:
            if (highlight > 0) highlight--;
            if (highlight < scroll_pos) scroll_pos--;
            break;
        case 'j':
        case KEY_DOWN:
            if (highlight + 1 < count) highlight++;
            if (highlight + 3 >= scroll_pos + height) scroll_pos++;
            break;
        case 'h':
        case KEY_LEFT:
            if (scroll_pos - list_height > 0) {
                scroll_pos -= list_height;
                highlight -= list_height;
            } else {
                scroll_pos = 0;
                highlight = 0;
            }
            break;
        case 'l':
        case KEY_RIGHT:
            if (scroll_pos + list_height <= count - list_height) {
                scroll_pos += list_height;
                highlight += list_height;
            } else if (scroll_pos + list_height > count - list_height) {
                scroll_pos = count - list_height;
                highlight = count - 1;
            } else {
                highlight = count - 1;
            }
            break;
        case KEY_MOUSE:
            if (getmouse(&interact) != 0) break;
            hlinteract = scroll_pos + interact.y - 2;
            if (scroll_pos > hlinteract ||
                (scroll_pos + list_height - 1) < hlinteract ||
                count < hlinteract) break;
            highlight = hlinteract;
            break;
        case 'c':
            snprintf(
                queue, sizeof(queue), " highlight = %d, scroll_pos = %d, count = %d ", highlight, scroll_pos, count);
            break;
        case 'g':
            attron(COLOR_PAIR(7) | A_REVERSE);
            mvprintw(LINES - 1, 0, "%-*s", COLS, "grep");
            echo();
            curs_set(1);
            c = mvwgetnstr(stdscr, LINES - 1, 5, input, sizeof(input));
            attroff(COLOR_PAIR(7) | A_REVERSE);
            curs_set(0);
            noecho();
            if (c == KEY_RESIZE) goto keyreturn;
            strmodquote(input);
            strcmdsecure(input);
            if (strisempty(input)) {
                if (!strcmp(grepfiles, "")) {
                    strcpy(queue, " None ");
                    break;
                }
                strcpy(grepfiles, "");
            } else
                snprintf(grepfiles, sizeof(grepfiles), "| grep %s", input);
            resetread(files, &count, &scroll_pos, &highlight, " (grep) Reloaded ");
            goto readagain;
        case 'i':
            metadata_window(files[highlight]);
            goto keyreturn;
        case 'M':
        case 'm':
            if (doMouse) {
                mousemask(0, NULL);
                strcpy(queue, " Disabled mouse/screen touch ");
            }
            else {
                mousemask(BUTTON1_PRESSED | BUTTON1_CLICKED | BUTTON1_RELEASED, NULL);
                strcpy(queue, " Enabled mouse/screen touch ");
            }
            doMouse = !doMouse;
            break;
        case 10:
        case 32:
        case 'p':
            if (isQueueMode) {
                addVectorMap(&playlistQueue, highlight, files[highlight]);
                snprintf(queue, sizeof(queue), " %s added to Queue ", basename(files[highlight]));
                break;
            }
            playaudio_fork(files[highlight]);
            snprintf(queue, sizeof(queue), " Playing: %-*s", width - 11, basename(files[highlight]));
            pclose(fp);
            break;
        case 'r':
            attron(COLOR_PAIR(7) | A_REVERSE);
            mvprintw(LINES - 1, 0, "%-*s", COLS, "Recursive N? ");
            echo();
            curs_set(1);
            c = mvwgetnstr(stdscr, LINES - 1, 13, input, sizeof(input));
            attroff(COLOR_PAIR(7) | A_REVERSE);
            curs_set(0);
            noecho();
            if (c == KEY_RESIZE) goto keyreturn;
            if (strisempty(input)) {
                isRecursive = !isRecursive;
                rcsv = 0;
            } else if (atoi(input) > 0)
                rcsv = atoi(input) + 1;
            memset(files, 0, sizeof(char *));
            count = 0;
            scroll_pos = 0;
            highlight = 0;
            resetread(files, &count, &scroll_pos, &highlight, "");
            // clang-format off
            snprintf(queue, sizeof(queue), " (recursive=%s) Reloaded ", (isRecursive ? "true" : "false"));
            // clang-format on
            goto readagain;
        case 's':
            if (isQueueMode) {
                c = deleteVectorMap(&playlistQueue, highlight);
                if (!c) {
                    c = 0;
                    break;
                }
                qm = 0;
                snprintf(
                    queue, sizeof(queue), " %s removed from Queue ", basename(files[highlight]));
                c = 0;
                break;
            }
            PLAYAUDIO_KILL();
            playaudio_pid = -1;
            strcpy(queue, " Stopped Music ");
            break;
        case 'v':
            attron(COLOR_PAIR(7) | A_REVERSE);
            mvprintw(LINES - 1, 0, "%-*s", COLS, "Sorted [A/D/S]?");
            echo();
            curs_set(1);
            c = mvwgetnstr(stdscr, LINES - 1, 16, input, sizeof(input));
            attroff(COLOR_PAIR(7) | A_REVERSE);
            curs_set(0);
            noecho();
            if (c == KEY_RESIZE) goto keyreturn;
            if (strisempty(input) || !strcasecmp(input, "S")) {
                if (!strcmp(sortfiles, "shuf")) break;
                strcpy(sortfiles, "shuf");
                resetread(files, &count, &scroll_pos, &highlight, " (shuf) Reloaded ");
                goto readagain;
            }
            strcpy(sortfiles, "sort -V");
            if (!strcasecmp(input, "D"))
                strncat(sortfiles, "r", sizeof(sortfiles) - strlen(sortfiles) - 1);
            resetread(files, &count, &scroll_pos, &highlight, " (sort) Reloaded ");
            goto readagain;
        case 'W':
        case 'w':
            if (isQueueMode) {
                if (playlistQueue.size < 1) {
                    strcpy(queue, " Queue is empty ");
                    break;
                }
                PLAYAUDIO_KILL();
                while (1) {
                    loopretval = loopitem_window(queue_win, playlistQueue.data[qm].value, 2);
                    if (loopretval == 33280) { break; }
                    if (loopretval == 1) {
                        qm = (qm == 0) ? playlistQueue.size - 1 : qm - 1;
                    } else {
                        qm = (qm == (int)playlistQueue.size - 1) ? 0 : qm + 1;
                    }
                }
                highlight = playlistQueue.data[qm].key;
                goto keyreturn;
            }
            loop_window(files[highlight], &highlight, files, count);
            goto keyreturn;
        case 'Y':
        case 'y':
            isQueueMode = !isQueueMode;
            if (isQueueMode == 0) {
                clearVectorMap(&playlistQueue);
                qm = 0;
                strcpy(queue, " Exited Queue Mode ");
                break;
            }
            initVectorMap(&playlistQueue, 1);
            strcpy(queue, " Custom Queue Mode | [p] Add, [s] Remove | [w] Play ");
            break;
        case 'H':
        case '?':
            help_window();
            goto keyreturn;
        case 'C':
            if (!use_cava_check()) {
                strcpy(queue, " Install {ffmpeg/sox, pv, cava} first ");
                break;
            }
            use_cava = !use_cava;
            if (use_cava)
                strcpy(queue, " (cava) Enabled for Loop Mode ");
            else
                strcpy(queue, " (cava) Disabled for Loop Mode ");
            break;
        case 'D':
            attron(COLOR_PAIR(7) | A_REVERSE);
            mvprintw(LINES - 1, 0, "%-*s", COLS, "Change Directory: ");
            echo();
            curs_set(1);
            c = mvwgetnstr(stdscr, LINES - 1, 18, directory, sizeof(directory));
            attroff(COLOR_PAIR(7) | A_REVERSE);
            curs_set(0);
            noecho();
            if (c == KEY_RESIZE) goto keyreturn;
            strmodquote(directory);
            strcmdsecure(directory);
            snprintf(cmd, sizeof(cmd), "echo -n \"%s\" 2>/dev/null", directory);
            fp = popen(cmd, "r");
            if (fp == NULL) {
                strcpy(queue, " Unable to read input ");
                break;
            }
            if (!strisempty(directory)) {
                fgets(cmd, sizeof(cmd), fp);
                if (realpath(cmd, directory) == NULL) strcpy(directory, cmd);
            } else
                strcpy(directory, "(null)");
            pclose(fp);
            if (chdir(directory) != 0) {
                snprintf(queue, sizeof(queue), " %s not a directory ", directory);
                break;
            }
            resetread(files, &count, &scroll_pos, &highlight, " Changed Directory ");
            goto readagain;
        case 'E':
            popup_error("GODDESS ON EARTH, ALL THY SOULS MAY FIND THEIR REST...\n- ???");
        case 'N':
            if (!strcmdv("termux-notification")) break;
            if (strcmp(NOTIF_ID, "NOTIF-QPA")) {
                snprintf(cmd, sizeof(cmd), "termux-notification-remove %s &", NOTIF_ID);
                system(cmd);
                strcpy(NOTIF_ID, "NOTIF-QPA");
                strcpy(queue, " Notification removed ");
            } else {
                snprintf(NOTIF_ID, sizeof(NOTIF_ID), "%s_%d", cmdname, currentpid);
                snprintf(cmd, sizeof(cmd),
                    "termux-notification "
                    "--id '%1$s' "
                    "--title 'Quilantia Music Player' "
                    "--content 'Active Process: %2$d' "
                    "--icon 'music_note' "
                    "--button1 'Next' "
                    "--button1-action '%3$s' "
                    "--button2 'Stop' "
                    "--button2-action '%3$s; kill %2$d' "
                    "--on-delete 'termux-notification-remove %1$s' "
                    "--priority 'high' "
                    "--ongoing &",
                    NOTIF_ID, currentpid, PKILLPA);
                system(cmd);
                snprintf(queue, sizeof(queue), " termux-notification = %s", NOTIF_ID);
            }
            break;
        case 'R':
            resetread(files, &count, &scroll_pos, &highlight, " (user) Reloaded ");
            goto readagain;
        case 'T':
            snprintf(queue, sizeof(queue), " Terminal Size: %d x %d ", COLS, LINES);
            break;
        case 'X':
            if (deleteFileWin(files[highlight]) != 1) goto keyreturn;
            if (count - 1 <= highlight) highlight = count - 2;
            resetread(files, &count, &nullint, &nullint, NULL);
            goto readagain;
            break;
        case 'Q':
        case 'q':
            PLAYAUDIO_KILL();
            if (strcmdv("termux-notification") && strcmp(NOTIF_ID, "NOTIF-QPA"))
                snprintf(buffer, sizeof(buffer), "termux-notification-remove %s &", NOTIF_ID);
            system(buffer);
            goto endmain;
        }
    }
keyreturn:
    flushinp();
    clear();
    delwin(playlist_pad);
    while (COLS < MIN_WIDTH || LINES < MIN_HEIGHT) {
        mvprintw(0, 0,
            "Terminal size too small. Resize to properly display %s, press 'q' to exit%-512s",
            cmdname, ".");
        refresh();
        if (getch() == 'q') {
            main_retval = 1;
            PLAYAUDIO_KILL();
            goto endmain;
        }
    }
    if (highlight >= scroll_pos + list_height) {
        scroll_pos = highlight - list_height + 1;
    }
    if (highlight < scroll_pos) {
        scroll_pos = highlight;
    }
    goto refreshmain;
endmain:
    for (int i = 0; i < count; i++)
        free(files[i]);
    pclose(fp);
    if (isQueueMode) clearVectorMap(&playlistQueue);
    endwin();
    return main_retval;
}
