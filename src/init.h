#ifndef INIT_H
#define INIT_H

#include <stdlib.h>

struct InitQueue {
    char *dirPath;
    struct InitQueue *next;
};

int enqueue(struct InitQueue **queue, const char *dirPath);
int dequeue(struct InitQueue **queue, char *outPath, size_t outPathLen);
int handleInit(char *rootDir);

#endif
