#ifndef PROCESS_H
#define PROCESS_H

#include "args.h"
#include "errors.h"
#include <stdio.h>

enum FileType {
    FILE_TYPE_UNKNOWN,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_REGULAR
};

struct IndexState {
    char *curIndexFileDir;
    FILE *curIndexFile;
};

struct FileIterator {
    struct IndexState *stack; // pointer == array
    size_t stackSize;
    size_t stackMax;
    enum FileIteratorStatus status;
};

struct ProcessContext {
    char *currentFilePath;
    char *outPath;
    FILE *outFile;
    enum FileType currentFileType;
    enum ProcessContextStatus status;
};

struct ProjectState {
    struct Arguments args;
    struct ProcessContext context;
    struct FileIterator iter;
    enum FileHandlerStatus (*handlerFunction)(struct ProcessContext *context);
    enum ProjectStateStatus status;
};

enum IndexStateStatus appendIndexState(struct FileIterator *iter,
                                       const char *indexFilePath);
struct IndexState *popIndexState(struct FileIterator *iter);
enum FileHandlerStatus handleInit(struct ProcessContext *context);
enum FileHandlerStatus handleCheck(struct ProcessContext *context);
enum FileHandlerStatus handleList(struct ProcessContext *context);
enum FileHandlerStatus handleCollate(struct ProcessContext *context);
enum FileIteratorStatus getNextFile(struct FileIterator *iter,
                                    struct ProcessContext *context);
int processProject(struct Arguments *args);

#endif
