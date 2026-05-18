#ifndef STRINGCUSTOM_H
#define STRINGCUSTOM_H

#include <bits/strcasecmp.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NOTSTR "N/A"

// clang-format off
// String is NULL
static const char *strisnull(char *string, const char *placeholder) {
    if (string == NULL || !strlen(string)) return placeholder;
    else return string;
}

// String is empty
static int strisempty(const char *s) {
    if (s == NULL) return 1;
    while (*s) {
        if (!isspace((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

// String newline count
static int strlflen(const char *s) {
    int c = 0;
    for (; *s; s++) c += (*s == '\n');
    return c;
}

// String modify quotes
static void strmodquote(char *s) {
    int ind = -1, c = 0;
    for (int i = 0; i < (int)strlen(s); i++) {
        if (s[i] == '"') {
            ind = i;
            c++;
        }
    }
    if (c % 2 != 0 && ind != -1)
    memmove(&s[ind], &s[ind + 1], strlen(s) - ind);
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

// String command check
static int strcmdv(const char *cmd) {
    char *path = getenv("PATH");
    if (!path) return 0;
    char *path_copy = strdup(path);
    char *dir = strtok(path_copy, ":");
    char full_path[1024];
    while (dir != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);
        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return 1;
        }
        dir = strtok(NULL, ":");
    }
    free(path_copy);
    return 0;
}

// Integer minimum
static int intmin(int a, int b) {
    if (a > b) return b;
    else return a;
}

/// VectorMap
typedef struct {
    int key;
    char *value;
} Map;

typedef struct {
    Map *data;
    size_t size;
    size_t capacity;
} VectorMap;

static void initVectorMap(VectorMap *a, int initialCapacity) {
    a->data = malloc(initialCapacity * sizeof(Map));
    a->size = 0;
    a->capacity = initialCapacity;
}

static void addVectorMap(VectorMap *a, int key, char *value) {
    if (a->size == a->capacity) {
        a->capacity *= 2; 
        a->data = realloc(a->data, a->capacity * sizeof(Map));
    }
    a->data[a->size].key = key;
    a->data[a->size].value = value;
    a->size++;
}

static int hasVectorMapKey(VectorMap *a, int key) {
    for (size_t i = 0; i < a->size; i++) {
        if (a->data[i].key == key) return 1;
    }
    return 0;
}

static int deleteVectorMap(VectorMap *a, int key) {
    size_t ind = -1;
    for (size_t i = 0; i < a->size; i++) {
        if (a->data[i].key == key) {
            ind = i;
            break;
        }
    }
    if (ind == (size_t)-1) return 0;
    for (size_t i = ind; i < a->size - 1; i++) {
        a->data[i] = a->data[i + 1];
    }
    a->size--;
    return 1;
}

static void clearVectorMap(VectorMap *a) {
    free(a->data);
    a->data = NULL;
    a->size = 0;
    a->capacity = 0;
}

#endif
