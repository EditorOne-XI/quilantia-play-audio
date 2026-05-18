#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

// String has extension
static int strext(char *string, const char *ext) {
    char *getext = strrchr(string, '.');
    if (!strcasecmp(getext, ext)) return 1;
    return 0;
}

// String command secure (anti-injection)
static void strcmdsecure(char *str) {
    if (!str) return;
    int cmd_sub = 0;
    int in_quotes = 0;
    char quote_char = 0;
    for (char *p = str; *p; p++) {
        if ((*p == '"' || *p == '\'') && (p == str || *(p - 1) != '\\')) {
            if (!in_quotes) {
                in_quotes = 1;
                quote_char = *p;
            } else if (*p == quote_char) {
                in_quotes = 0;
            }
        }
        if (quote_char != '\'' && *p == '$' && p[1] == '(') {
            cmd_sub++;
            p++;
            continue;
        }
        if (cmd_sub > 0 && *p == ')') {
            strcpy(str, "");
            return;
        }
        if (!in_quotes && (*p == '|' || *p == ';' || *p == '&')) {
            *p = '\0';
            break;
        }
    }
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

int main(int argc, char *argv[])
{
    char input[] = "grep -iE \"'chicken|jockey'\" $(whoami).txt | awk '{print $1}'";
    // scanf(" %[^\n]", input);
    printf("First Str: %s\n", input);
    strcmdsecure(input);
    printf("Second Str: %s\n", input);
    return 0;
}
