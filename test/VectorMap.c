#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/***************************************
 * TODO:
 *  change playlist directory (done)
 *  reload items (done)
 *  getopts (done)
 *  exclude items (done)
 *  recursive playlist (done)
 *  custom queue (done)
 ***************************************/

// clang-format off
typedef struct {
    int key;
    char *value;
} Map;

typedef struct {
    Map *data;
    size_t size;
    size_t capacity;
} VectorMap;

void initVectorMap(VectorMap *a, int initialCapacity) {
    a->data = malloc(initialCapacity * sizeof(Map));
    a->size = 0;
    a->capacity = initialCapacity;
}

void addVectorMap(VectorMap *a, int key, char *value) {
    if (a->size == a->capacity) {
        a->capacity *= 2; 
        a->data = realloc(a->data, a->capacity * sizeof(Map));
    }
    a->data[a->size].key = key;
    a->data[a->size].value = value;
    a->size++;
}

int hasVectorMapKey(VectorMap *a, int key) {
    for (size_t i = 0; i < a->size; i++) {
        if (a->data[i].key == key) return 1;
    }
    return 0;
}

char *getVectorMapKey(VectorMap *a, int key) {
    for (size_t i = 0; i < a->size; i++) {
        if (a->data[i].key == key)
            return a->data[i].value;
    }
    return NULL;
}

int deleteVectorMap(VectorMap *a, int key) {
    size_t ind = -1;
    for (size_t i = 0; i < a->size; i++) {
        if (a->data[i].key == key) {
            ind = i;
            break;
        }
    }
    if (ind == -1) return 0;
    for (size_t i = ind; i < a->size - 1; i++) {
        a->data[i] = a->data[i + 1];
    }
    a->size--;
    return 1;
}

void clearVectorMap(VectorMap *a) {
    free(a->data);
    a->data = NULL;
    a->size = 0;
    a->capacity = 0;
}
// clang-format on

int main(int argc, char *argv[]) {
    printf("sizeof Map = %zu\n", sizeof(Map));
    return EXIT_SUCCESS;
}
