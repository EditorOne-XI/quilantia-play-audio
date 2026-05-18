#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **strsplit(char *string, char *delim, int *count) {
    char **arr = malloc(sizeof(char *));
    char *temp = strtok(string, delim);
    size_t i = 0;
    while (temp != NULL) {
        if (i > 0) arr = realloc(arr, sizeof(char *) * (i + 1));
        arr[i++] = temp;
        temp = strtok(NULL, delim);
    }
    *count = i;
    return arr;
}

int main(int argc, char *argv[]) {
    char str[] = "The,First,Goddess,On Earth";
    int c = 0;
    char **me = strsplit(str, ",", &c);
    for (int i = 0; i < c; i++) {
        printf("%d: %s\n", i, me[i]);
    }
    free(me);
    return 0;
}
