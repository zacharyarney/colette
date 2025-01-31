#include "constants.h"
#include "errors.h"
#include "files.h"
#include "process.h"
#include "reporting.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bool isName(const char *fileName) {
    // skip blank lines and comments
    return fileName[0] != '\n' && fileName[0] != '#';
}

static void freeProcessContext(struct ProcessContext *context) {
    if (!context) {
        return;
    }

    if (context->currentFilePath) {
        free(context->currentFilePath);
        context->currentFilePath = NULL;
    }
    if (context->outPath) {
        free(context->outPath);
        context->outPath = NULL;
    }
    if (context->outFile) {
        fclose(context->outFile);
    }
}

static void freeIndexState(struct IndexState *indexState) {
    if (!indexState) {
        return;
    }

    if (indexState->curIndexFileDir) {
        free(indexState->curIndexFileDir);
        indexState->curIndexFileDir = NULL;
    }
    if (indexState->curIndexFile) {
        fclose(indexState->curIndexFile);
    }
}

static void freeProjectState(struct ProjectState *state) {
    if (!state) {
        return;
    }

    if (state->iter.stack != NULL) {
        free(state->iter.stack);
        state->iter.stack = NULL;
    }

    freeProcessContext(&state->context);
}

static struct ProcessContext initProcessContext(void) {
    struct ProcessContext context = {.currentFilePath =
                                         malloc(COLETTE_PATH_BUF_SIZE),
                                     .outPath = malloc(COLETTE_PATH_BUF_SIZE),
                                     .outFile = NULL,
                                     .status = CTX_SUCCESS};

    if (!context.currentFilePath || !context.outPath) {
        free(context.currentFilePath);
        free(context.outPath);
        context.currentFilePath = NULL;
        context.outPath = NULL;
        context.currentFileType = FILE_TYPE_UNKNOWN;
        context.status = CTX_FAILURE;
    }

    return context;
}

static struct FileIterator initFileIterator(void) {
    struct FileIterator iter;
    iter.status = ITER_SUCCESS;
    iter.stackSize = 0;
    iter.stackMax = COLETTE_PROJECT_DEPTH;
    iter.stack = malloc(sizeof(struct IndexState) * iter.stackMax);
    if (!iter.stack) {
        iter.status = ITER_FAILURE;
        return iter;
    }

    return iter;
}

static struct ProjectState initProjectState(struct Arguments *args) {
    struct ProjectState state = {0};
    struct FileIterator iter = initFileIterator();
    struct ProcessContext context = initProcessContext();

    state.args = *args;
    state.iter = iter;
    state.context = context;
    state.status = STATE_SUCCESS;

    return state;
}

static int setCurrentFilePath(struct ProcessContext *context,
                              const char *dirPath,
                              const char *fileName) {
    if (!context || !dirPath || !fileName) {
        reportProcessError(PROCESS_OP_CTX_PATH, NULL, PROC_ERR_INVALID_STATE);
        return -1;
    }

    if (context->currentFilePath) {
        free(context->currentFilePath);
        context->currentFilePath = NULL;
    }

    size_t dirPathLen = strlen(dirPath);
    size_t fileNameLen = strlen(fileName);
    int extraChars = handlePathBufTrailingSlashPad(dirPath, dirPathLen);
    size_t baseFilePathLen = dirPathLen + fileNameLen + extraChars;
    if (baseFilePathLen > COLETTE_MAX_PATH_LEN) {
        reportProcessError(
            PROCESS_OP_CTX_PATH, dirPath, PROC_ERR_PATH_TOO_LONG);
        return -1;
    }

    char baseFilePath[baseFilePathLen];
    if (joinPath(baseFilePath, baseFilePathLen, dirPath, fileName) !=
        PATH_SUCCESS) {
        return -1;
    }

    size_t extensionLen = COLETTE_NAME_BUF_SIZE;
    size_t resolvedPathLen = baseFilePathLen + extensionLen;
    char *resolvedPath = malloc(resolvedPathLen);
    if (!resolvedPath) {
        reportProcessError(PROCESS_OP_CTX_PATH, dirPath, PROC_ERR_MEMORY_ALLOC);
        return -1;
    }

    enum ResolveStatus resolvedPathStatus =
        resolveFile(resolvedPath, resolvedPathLen, baseFilePath);

    switch (resolvedPathStatus) {
    case RESOLVE_DIR:
        context->currentFilePath = resolvedPath;
        context->currentFileType = FILE_TYPE_DIRECTORY;
        return 0;
    case RESOLVE_EXACT:
    case RESOLVE_FILE:
        context->currentFilePath = resolvedPath;
        context->currentFileType = FILE_TYPE_REGULAR;
        return 0;
    case RESOLVE_LINK:
        reportProcessError(
            PROCESS_OP_CTX_PATH, baseFilePath, PROC_ERR_INVALID_LINK);
        free(resolvedPath);
        return -1;
    case RESOLVE_ERROR:
        reportProcessError(
            PROCESS_OP_CTX_PATH, baseFilePath, PROC_ERR_INVALID_PATH);
        free(resolvedPath);
        return -1;
    case RESOLVE_NO_ACCESS:
        reportProcessError(
            PROCESS_OP_CTX_PATH, baseFilePath, PROC_ERR_ACCESS_DENIED);
        free(resolvedPath);
        return -1;
    case RESOLVE_NOT_FOUND:
        reportProcessError(
            PROCESS_OP_CTX_PATH, baseFilePath, PROC_ERR_FILE_NOT_FOUND);
        free(resolvedPath);
        return -1;
    default:
        reportProcessError(
            PROCESS_OP_CTX_PATH, baseFilePath, PROC_ERR_INVALID_STATE);
        free(resolvedPath);
        return -1;
    }
}

static int setHandlerFunction(struct Arguments *args,
                              struct ProjectState *state) {
    switch (args->mode) {
        //     case MODE_COLLATE:
        //         state->handlerFunction = handleCollate;
        //         break;
        //     case MODE_LIST:
        //         state->handlerFunction = handleList;
        //         break;
    case MODE_CHECK:
        state->handlerFunction = handleCheck;
        break;
    default:
        reportProcessError(
            PROCESS_OP_STATE_MODE, args->directory, PROC_ERR_INVALID_MODE);
        return -1;
    }

    return 0;
}

static FILE *openIndexFile(const char *indexFileDir) {
    char indexFilePath[COLETTE_PATH_BUF_SIZE];
    int joinStatus;
    if ((joinStatus = joinPath(
             indexFilePath, sizeof(indexFilePath), indexFileDir, ".index")) !=
        PATH_SUCCESS) {
        return NULL;
    }

    FILE *indexFile = fopen(indexFilePath, "r");
    if (!indexFile) {
        return NULL;
    }

    return indexFile;
}

enum IndexStateStatus appendIndexState(struct FileIterator *iter,
                                       const char *indexFileDir) {
    if (!iter || !indexFileDir) {
        reportProcessError(PROCESS_OP_ITER_PUSH, NULL, PROC_ERR_INVALID_STATE);
        return INDEX_MEMORY_ERROR;
    }

    size_t pathLen = strlen(indexFileDir) + 1;
    if (pathLen >= COLETTE_PATH_BUF_SIZE) {
        reportProcessError(
            PROCESS_OP_ITER_PUSH, indexFileDir, PROC_ERR_PATH_TOO_LONG);
        return INDEX_PATH_TOO_LONG;
    }

    if (iter->stackSize >= iter->stackMax) {
        if (iter->stackMax >= SIZE_MAX / 2) {
            reportProcessError(
                PROCESS_OP_ITER_PUSH, indexFileDir, PROC_ERR_STACK_OVERFLOW);
            return INDEX_OVERFLOW_ERROR;
        }
        if (iter->stackMax > COLETTE_PROJECT_DEPTH / 2) {
            fprintf(stderr, "DEBUG -- iter->stackMAx > COLETTE_PROJECT_DEPTH / 2");
            reportProcessError(
                PROCESS_OP_ITER_NEXT, indexFileDir, PROC_ERR_TOO_DEEP);
            return INDEX_TOO_LARGE;
        }

        size_t newMax = iter->stackMax * 2;
        // enforce max stack depth
        if (newMax < iter->stackMax) { // Integer overflow occurred
            fprintf(stderr, "DEBUG -- newMax < iter->stackMax");
            reportProcessError(
                PROCESS_OP_ITER_NEXT, indexFileDir, PROC_ERR_TOO_DEEP);
            return INDEX_TOO_LARGE;
        }

        // convert max count to bytes
        size_t newSizeInBytes = newMax * sizeof(struct IndexState);
        if (newSizeInBytes / sizeof(struct IndexState) !=
            newMax) { // Overflow check
            fprintf(stderr, "DEBUG -- newSizeInBytes / sizeof(struct IndexState) != newMax");
            reportProcessError(
                PROCESS_OP_ITER_NEXT, indexFileDir, PROC_ERR_TOO_DEEP);
            return INDEX_TOO_LARGE;
        }
        struct IndexState *newStack =
            realloc(iter->stack, newSizeInBytes);
        if (!newStack) {
            reportProcessError(
                PROCESS_OP_ITER_PUSH, indexFileDir, PROC_ERR_MEMORY_ALLOC);
            return INDEX_MEMORY_ERROR;
        }

        iter->stack = newStack;
        iter->stackMax = newMax;
    }

    char *pathCopy = malloc(pathLen);
    if (!pathCopy) {
        reportProcessError(
            PROCESS_OP_ITER_PUSH, indexFileDir, PROC_ERR_MEMORY_ALLOC);
        return INDEX_MEMORY_ERROR;
    }
    memcpy(pathCopy, indexFileDir, pathLen);

    FILE *indexFile = openIndexFile(pathCopy);
    if (!indexFile) {
        reportProcessError(
            PROCESS_OP_ITER_PUSH, indexFileDir, PROC_ERR_INDEX_MISSING);
        free(pathCopy);
        return INDEX_FILE_ERROR;
    }

    struct IndexState newState = {.curIndexFileDir = pathCopy,
                                  .curIndexFile = indexFile};

    iter->stack[iter->stackSize] = newState;
    iter->stackSize++;

    return INDEX_SUCCESS;
}

struct IndexState *popIndexState(struct FileIterator *iter) {
    if (!iter) {
        return NULL;
    }
    if (!iter->stack) {
        return NULL;
    }
    if (iter->stackSize < 1) {
        return NULL;
    }

    struct IndexState *poppedItem = &iter->stack[iter->stackSize - 1];
    iter->stackSize--;

    return poppedItem;
}

enum FileIteratorStatus getNextFile(struct FileIterator *iter,
                                    struct ProcessContext *context) {
    if (!iter || !context) {
        reportProcessError(PROCESS_OP_ITER_NEXT, NULL, PROC_ERR_INVALID_STATE);
        return ITER_FAILURE;
    }
    if (iter->stackSize < 1) {
        reportProcessError(
            PROCESS_OP_ITER_NEXT, NULL, PROC_ERR_INVALID_SEQUENCE);
        return ITER_END;
    }

    struct IndexState curIndexState = iter->stack[iter->stackSize - 1];
    char curFileName[COLETTE_NAME_BUF_SIZE];
    if (fgets(curFileName, sizeof(curFileName), curIndexState.curIndexFile)) {
        curFileName[strcspn(curFileName, "\n")] = '\0';
        if (isName(curFileName) && isIncluded(curFileName)) {
            if (setCurrentFilePath(
                    context, curIndexState.curIndexFileDir, curFileName) != 0) {
                return ITER_FAILURE;
            }

            if (context->currentFileType == FILE_TYPE_DIRECTORY) {
                if (appendIndexState(iter, context->currentFilePath) !=
                    INDEX_SUCCESS) {
                    return ITER_FAILURE;
                }

                return getNextFile(iter, context);
            }
        } else {
            return getNextFile(iter, context);
        }

    } else {
        struct IndexState *poppedIndex = popIndexState(iter);
        freeIndexState(poppedIndex);

        if (iter->stackSize) {
            return getNextFile(iter, context);
        } else {
            return ITER_END;
        }
    }

    return ITER_SUCCESS;
}

enum FileHandlerStatus handleCheck(struct ProcessContext *context) {
    if (!context) {
        return HANDLER_FAILURE;
    }

    errno = 0;
    FILE *file = fopen(context->currentFilePath, "r");
    if (!file) {
        if (errno == EACCES) { // we don't have permission -- unexpected
            reportProcessError(PROCESS_OP_HANDLE_CHECK,
                               context->currentFilePath,
                               PROC_ERR_ACCESS_DENIED);
            return HANDLER_FAILURE;
        }

        reportProcessError(PROCESS_OP_HANDLE_CHECK,
                           context->currentFilePath,
                           PROC_ERR_OPEN_FILE);
        return HANDLER_FAILURE;
    }

    fclose(file);

    return HANDLER_SUCCESS;
}

// enum FileHandlerStatus handleCollate(struct ProcessContext *context) {
//     errno = 0;
//     char inFileBaseName[COLETTE_NAME_BUF_SIZE + 1];
//     if (getBasename(path, inFileBaseName, sizeof(inFileBaseName) != 0)) {
//         return -1;
//     }
//
//     errno = 0;
//     FILE *inFileP = fopen(path, "r");
//     char inBuffer[8192];
//     size_t bytesRead;
//     if (!inFileP) {
//         reportFileError(FILE_OP_OPEN, path);
//         return -1;
//     }
//
//     errno = 0;
//     FILE *outFile = fopen(outFilePath, "w");
//     if (!outFile) {
//         reportFileError(FILE_OP_CREATE, outFilePath);
//         return -1;
//     }
//
//     errno = 0;
//     while ((bytesRead = fread(inBuffer, 1, sizeof(inBuffer), inFileP)) > 0) {
//         size_t bytesWritten = fwrite(inBuffer, 1, sizeof(bytesRead),
//         outFile); if (bytesWritten != bytesRead) {
//             reportFileError(FILE_OP_WRITE, path);
//             fclose(inFileP);
//             fclose(outFile);
//             return -1;
//         }
//     }
//
//     if (ferror(inFileP)) {
//         reportFileError(FILE_OP_READ, path);
//     }
//
//     fprintf(outFile, "\n");
//     fclose(inFileP);
//     fclose(outFile);
//
//     return 0;
// }

int processProject(struct Arguments *args) {
    struct ProjectState state = initProjectState(args);
    if (state.status != STATE_SUCCESS) {
        reportProcessError(
            PROCESS_OP_STATE_INIT, args->directory, PROC_ERR_INVALID_STATE);
        return -1;
    }
    if (state.iter.status == ITER_FAILURE ||
        state.context.status == CTX_FAILURE) {
        reportProcessError(
            PROCESS_OP_ITER_INIT, args->directory, PROC_ERR_MEMORY_ALLOC);
        state.status = STATE_FAILURE;
        return -1;
    }

    if (appendIndexState(&state.iter, args->directory) != INDEX_SUCCESS) {
        return -1;
    }
    if (setHandlerFunction(args, &state)) {
        return -1;
    }

    while ((state.iter.status = getNextFile(&state.iter, &state.context)) !=
           ITER_END) {
        if (state.iter.status == ITER_FAILURE) {
            return -1;
        }
        if (args->initMode) {
            // handleInit
        } else if (state.handlerFunction) {
            enum FileHandlerStatus handlerStatus =
                state.handlerFunction(&state.context);
            if (handlerStatus == HANDLER_FAILURE) {
                return -1;
            }
        }
    }
    // DON'T FORGET TO FREE STATE
    freeProjectState(&state);

    return 0;
}
