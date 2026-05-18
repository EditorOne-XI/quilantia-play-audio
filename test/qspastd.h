// clang-format off
#ifndef QSPASTD_H
#define QSPASTD_H

#include <asm-generic/fcntl.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#define FINDCMD_FFORMATS "3gp|aac|flac|m4a|mp3|ogg|opus|wav"
#define ERRMSG_FFORMATS "*.{3gp,aac,flac,m4a,mp3,ogg,opus,wav}"
#define PKILLPA "pkill -n play-audio >/dev/null 2>&1"
static pid_t audio_pid = -1;

static void AUDIO_KILL() {
    if (audio_pid > 0) kill(audio_pid, SIGTERM);
    else system(PKILLPA);
}

static const char *menukeys[17][2] = {
    {"up, k", "move cursor up"},
    {"down, j", "move cursor down"},
    {"left, h", "move cursor left"},
    {"right, l", "move cursor right"},
    {"i", "show audio info"},
    {"space, p", "play music"},
    {"s", "stop music"},
    {"g", "(grep) filter files"},
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

static void playmusic_fork(char *item) {
    if (audio_pid > 0) AUDIO_KILL();
    audio_pid = fork();
    if (audio_pid < 0) return;
    if (audio_pid == 0) {
        int fdin = open("/dev/null", O_RDONLY);
        if (fdin != -1) {
            dup2(fdin, STDIN_FILENO);
            close(fdin);
        }
        int fdout = open("/dev/null", O_WRONLY);
        if (fdout != -1) {
            dup2(fdout, STDOUT_FILENO);
            dup2(fdout, STDERR_FILENO);
            close(fdout);
        }
        char *args[] = {"play-audio", "-s", "media", item, NULL};
        execvp(args[0], args);
        audio_pid = -1;
        _exit(1);
    }
}

#endif
