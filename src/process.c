#include "constants.h"
#include "errors.h"
#include "files.h"
#include "process.h"
#include "reporting.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

static struct ProjectState initProjectState(void) {
    struct ProjectState state = {0};
    struct FileIterator iter = initFileIterator();
    struct ProcessContext context = initProcessContext();

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
    if (joinPath(baseFilePath, baseFilePathLen, dirPath, fileName) != 0) {
        return -1;
    }

    size_t extensionLen = COLETTE_EXT_BUF_SIZE;
    size_t resolvedPathLen = baseFilePathLen + extensionLen;
    if (resolvedPathLen > COLETTE_MAX_PATH_LEN) {
        reportProcessError(
            PROCESS_OP_CTX_PATH, dirPath, PROC_ERR_PATH_TOO_LONG);
        return -1;
    }

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

static int setOutput(struct Arguments *args, struct ProjectState *state) {
    if (!args || !state) {
        reportProcessError(
            PROCESS_OP_CTX_OUTPUT, args->directory, PROC_ERR_INVALID_STATE);
        return -1;
    }

    if (args->mode == MODE_CHECK) {
        return 0;
    }

    if (state->context.outPath) {
        free(state->context.outPath);
        state->context.outPath = NULL;
    }

    size_t dirPathLen = strlen(args->directory);
    size_t titleLen = strlen(args->title);
    int extraChars = handlePathBufTrailingSlashPad(args->directory, dirPathLen);
    size_t outPathLen = dirPathLen + titleLen + extraChars;
    if (outPathLen > COLETTE_MAX_PATH_LEN) {
        reportProcessError(
            PROCESS_OP_CTX_OUTPUT, args->directory, PROC_ERR_PATH_TOO_LONG);
        return -1;
    }

    char *outPath = malloc(outPathLen);
    if (!outPath) {
        reportProcessError(
            PROCESS_OP_CTX_PATH, args->directory, PROC_ERR_MEMORY_ALLOC);
        return -1;
    }
    if (joinPath(outPath, outPathLen, args->directory, args->title) != 0) {
        free(outPath);
        return -1;
    }

    if (args->mode == MODE_LIST) {
        errno = 0;
        if (mkdir(outPath, 0777) != 0 && errno != EEXIST) {
            // if mkdir fails for a reason other than the directory existing
            reportProcessError(PROCESS_OP_CTX_OUTPUT,
                               args->directory,
                               PROC_ERR_INVALID_OUTPUT);
            free(outPath);
            return -1;
        }
        state->context.outPath = outPath;
    } else if (args->mode == MODE_COLLATE) {
        char extension_PLACEHOLDER[] = ".md";
        size_t extensionLen = strlen(extension_PLACEHOLDER) + 1;
        if (titleLen + extensionLen > COLETTE_NAME_BUF_SIZE) {
            reportProcessError(
                PROCESS_OP_CTX_PATH, args->directory, PROC_ERR_NAME_TOO_LONG);
            return -1;
        }

        size_t outFilePathLen = outPathLen + extensionLen;
        if (outFilePathLen > COLETTE_MAX_PATH_LEN) {
            reportProcessError(
                PROCESS_OP_CTX_PATH, args->directory, PROC_ERR_PATH_TOO_LONG);
            return -1;
        }

        size_t outFilePathBufSize = outPathLen + COLETTE_EXT_BUF_SIZE;
        char *outFilePath = malloc(outFilePathBufSize);
        if (!outFilePath) {
            reportProcessError(
                PROCESS_OP_CTX_PATH, args->directory, PROC_ERR_MEMORY_ALLOC);
            free(outPath);
            return -1;
        }
        if (outPathLen > outFilePathLen) {
            reportProcessError(
                PROCESS_OP_CTX_PATH, args->directory, PROC_ERR_MEMORY_ALLOC);
            free(outPath);
            return -1;
        }

        /* *
         * TODO: users will probably want to pick the output format and file
         * extension. For now we default to markdown because that's what I use.
         * txt could be a more practical default in the future when output is
         * configurable.
         * */
        if (joinExtension(outFilePath, outFilePathBufSize, outPath, ".md") !=
            0) {
            reportProcessError(PROCESS_OP_CTX_OUTPUT,
                               args->directory,
                               PROC_ERR_INVALID_OUTPUT);
            free(outPath);
            free(outFilePath);
            return -1;
        }
        state->context.outPath = outFilePath;

        errno = 0;
        FILE *outFile = fopen(state->context.outPath, "w");
        if (!outFile) {
            reportProcessError(PROCESS_OP_CTX_OUTPUT,
                               args->directory,
                               PROC_ERR_INVALID_OUTPUT);
            free(outPath);
            free(outFilePath);
            return -1;
        }
        state->context.outFile = outFile;

        free(outPath);
    } else {
        reportProcessError(
            PROCESS_OP_STATE_MODE, args->directory, PROC_ERR_INVALID_MODE);
        if (outPath) {
            free(outPath);
        }
        return -1;
    }

    return 0;
}

static FILE *openIndexFile(const char *indexFileDir) {
    char indexFilePath[COLETTE_PATH_BUF_SIZE];
    int joinStatus;
    if ((joinStatus = joinPath(
             indexFilePath, sizeof(indexFilePath), indexFileDir, ".index")) !=
        0) {
        return NULL;
    }

    FILE *indexFile = fopen(indexFilePath, "r");
    if (!indexFile) {
        return NULL;
    }

    return indexFile;
}

static int appendIndexState(struct FileIterator *iter, const char *indexFileDir) {
    if (!iter || !indexFileDir) {
        reportProcessError(PROCESS_OP_ITER_PUSH, NULL, PROC_ERR_INVALID_STATE);
        return -1;
    }

    size_t pathLen = strlen(indexFileDir) + 1;
    if (pathLen >= COLETTE_PATH_BUF_SIZE) {
        reportProcessError(
            PROCESS_OP_ITER_PUSH, indexFileDir, PROC_ERR_PATH_TOO_LONG);
        return -1;
    }

    if (iter->stackSize >= iter->stackMax) {
        if (iter->stackMax >= SIZE_MAX / 2) {
            reportProcessError(
                PROCESS_OP_ITER_PUSH, indexFileDir, PROC_ERR_STACK_OVERFLOW);
            return -1;
        }
        if (iter->stackMax > COLETTE_PROJECT_DEPTH / 2) {
            reportProcessError(
                PROCESS_OP_ITER_NEXT, indexFileDir, PROC_ERR_TOO_DEEP);
            return -1;
        }

        size_t newMax = iter->stackMax * 2;
        // enforce max stack depth
        if (newMax < iter->stackMax) { // Integer overflow occurred
            reportProcessError(
                PROCESS_OP_ITER_NEXT, indexFileDir, PROC_ERR_TOO_DEEP);
            return -1;
        }

        // convert max count to bytes
        size_t newSizeInBytes = newMax * sizeof(struct IndexState);
        if (newSizeInBytes / sizeof(struct IndexState) != newMax) {
            // Overflow check
            reportProcessError(
                PROCESS_OP_ITER_NEXT, indexFileDir, PROC_ERR_TOO_DEEP);
            return -1;
        }
        struct IndexState *newStack = realloc(iter->stack, newSizeInBytes);
        if (!newStack) {
            reportProcessError(
                PROCESS_OP_ITER_PUSH, indexFileDir, PROC_ERR_MEMORY_ALLOC);
            return -1;
        }

        iter->stack = newStack;
        iter->stackMax = newMax;
    }

    char *pathCopy = malloc(pathLen);
    if (!pathCopy) {
        reportProcessError(
            PROCESS_OP_ITER_PUSH, indexFileDir, PROC_ERR_MEMORY_ALLOC);
        return -1;
    }
    memcpy(pathCopy, indexFileDir, pathLen);

    FILE *indexFile = openIndexFile(pathCopy);
    if (!indexFile) {
        reportProcessError(
            PROCESS_OP_ITER_PUSH, indexFileDir, PROC_ERR_INDEX_MISSING);
        free(pathCopy);
        return -1;
    }

    struct IndexState newState = {.curIndexFileDir = pathCopy,
                                  .curIndexFile = indexFile};

    iter->stack[iter->stackSize] = newState;
    iter->stackSize++;

    return 0;
}

static struct IndexState *popIndexState(struct FileIterator *iter) {
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

static enum FileIteratorStatus getNextFile(struct FileIterator *iter,
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
                if (appendIndexState(iter, context->currentFilePath) != 0) {
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

static enum FileHandlerStatus handleCheck(struct ProcessContext *context) {
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

static enum FileHandlerStatus handleCollate(struct ProcessContext *context) {
    if (!context) {
        return HANDLER_FAILURE;
    }
    if (!context->outFile) {
        return HANDLER_FAILURE;
    }

    errno = 0;
    char inBuffer[COLETTE_FILE_BUF_SIZE];
    size_t bytesRead;
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

    errno = 0;
    while ((bytesRead = fread(inBuffer, 1, sizeof(inBuffer), file)) > 0) {
        size_t bytesWritten = fwrite(inBuffer, 1, bytesRead, context->outFile);
        if (bytesWritten != bytesRead) {
            reportFileError(FILE_OP_WRITE, context->currentFilePath);
            fclose(file);
            return HANDLER_FAILURE;
        }
    }

    if (ferror(file)) {
        reportFileError(FILE_OP_READ, context->currentFilePath);
        fclose(file);
        return HANDLER_FAILURE;
    }

    /* *
     * TODO: User should be able to configure the separator between files. This
     * could be a really useful feature for clearly delimiting chapters with 
     * multiple newlines or a horizontal line or even using the file name as a
     * header. 
     * */
    if (fprintf(context->outFile, "\n") != 1) {
        reportFileError(FILE_OP_WRITE, context->currentFilePath);
        fclose(file);
        return HANDLER_FAILURE;
    }

    fclose(file);
    return HANDLER_SUCCESS;
}

static int setHandlerFunction(struct Arguments *args,
                              struct ProjectState *state) {
    if (!args || !state) {
        reportProcessError(
            PROCESS_OP_STATE_MODE, args->directory, PROC_ERR_INVALID_STATE);
        return -1;
    }

    switch (args->mode) {
    case MODE_COLLATE:
        state->handlerFunction = handleCollate;
        break;
        // case MODE_LIST:
        //     state->handlerFunction = handleList;
        //     break;
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

int processProject(struct Arguments *args) {
    struct ProjectState state = initProjectState();
    if (state.status != STATE_SUCCESS) {
        reportProcessError(
            PROCESS_OP_STATE_INIT, args->directory, PROC_ERR_INVALID_STATE);
        freeProjectState(&state);
        return -1;
    }
    if (state.iter.status == ITER_FAILURE ||
        state.context.status == CTX_FAILURE) {
        reportProcessError(
            PROCESS_OP_ITER_INIT, args->directory, PROC_ERR_MEMORY_ALLOC);
        state.status = STATE_FAILURE;
        freeProjectState(&state);
        return -1;
    }

    if (appendIndexState(&state.iter, args->directory) != 0) {
        freeProjectState(&state);
        return -1;
    }
    if (setHandlerFunction(args, &state) != 0) {
        freeProjectState(&state);
        return -1;
    }
    if (setOutput(args, &state) != 0) {
        freeProjectState(&state);
        return -1;
    }

    while ((state.iter.status = getNextFile(&state.iter, &state.context)) !=
           ITER_END) {
        if (state.iter.status == ITER_FAILURE) {
            freeProjectState(&state);
            return -1;
        }
        if (args->initMode) {
            // handleInit
        }
        if (state.handlerFunction) {
            enum FileHandlerStatus handlerStatus =
                state.handlerFunction(&state.context);
            if (handlerStatus == HANDLER_FAILURE) {
                freeProjectState(&state);
                return -1;
            }
        }
    }
    // DON'T FORGET TO FREE STATE
    freeProjectState(&state);

    return 0;
}
