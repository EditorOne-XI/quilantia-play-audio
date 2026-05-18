#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void backgroundproc(char *cmd, char *args[]) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    } 
    
    if (pid == 0) {
        if (execvp(cmd, args) == -1) {
            perror("execvp");
        }
        exit(1);
    } 
    printf("Background process PID: %d\n", pid);
    printf("printf...\n");
}

int main(void) {
    char *args[] = {"play-audio", "/storage/emulated/0/Music/_files/Different Heaven - Nekozilla [NCS Release].mp3", NULL};

    backgroundproc(args[0], args);

    printf("Running Test...\n");
    for (int i = 0; i < 100; i++) {
        printf("%3d - Hello World!\n", i + 1);
        sleep(1);
    }
    return 0;
}
