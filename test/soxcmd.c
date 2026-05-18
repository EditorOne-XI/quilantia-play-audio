#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

pid_t sox_pid = -1;
static void playmusic_fork(char *item) {
    sox_pid = fork();
    if (sox_pid < 0) exit(1);
    if (sox_pid == 0) {
        char *args[] = {"sox", item, "-d", "-q", NULL};
        execvp(args[0], args);
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) exit(1);
    printf("Running 'sox --help' in 3 seconds...\n");
    sleep(3);
    playmusic_fork(argv[1]);
    printf("It reached here! Success!\n");
    printf("sox_pid = %d\n", sox_pid);
    return 0;
}
