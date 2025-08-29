#include "constants.h"
#include "errors.h"
#include "files.h"
#include "init.h"
#include "reporting.h"
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static bool entryExists(FILE *indexFile, const char *entryName) {
    if (!indexFile || !entryName) {
        return false;
    }

    char lineBuf[COLETTE_NAME_BUF_SIZE];
    bool exists = false;

    while (fgets(lineBuf, sizeof(lineBuf), indexFile)) {
        size_t lineLen = strlen(lineBuf);
        if (lineLen == 0 || lineBuf[0] == '#') {
            continue;
        }

        if (lineBuf[lineLen - 1] == '\n') {
            lineBuf[lineLen - 1] = '\0';
            lineLen--;
        }

        if (strcmp(lineBuf, entryName) == 0) {
            exists = true;
            break;
        }
    }

    return exists;
}

static int addToIndexFile(FILE *indexFile, const char *entryName) {
    if (!indexFile || !entryName) {
        return -1;
    }

    long curPos = ftell(indexFile);
    rewind(indexFile);

    if (!entryExists(indexFile, entryName)) {
        fseek(indexFile, 0, SEEK_END);

        // Add newline to end of file if necessary
        if (ftell(indexFile) > 0) {
            fseek(indexFile, -1, SEEK_END);
            char lastChar;
            if (fread(&lastChar, 1, 1, indexFile) == 1 && lastChar != '\n') {
                fprintf(indexFile, "\n");
            }

            fseek(indexFile, 0, SEEK_END);
        }

        fprintf(indexFile, "%s\n", entryName);
    } else {
        fseek(indexFile, curPos, SEEK_SET);
    }

    return 0;
}

static FILE *openOrCreateIndexFile(const char *dirPath) {
    if (!dirPath) {
        return NULL;
    }

    char indexFilePath[COLETTE_PATH_BUF_SIZE];
    if (joinPath(indexFilePath, sizeof(indexFilePath), dirPath, ".index") !=
        0) {
        reportProcessError(
            PROCESS_OP_HANDLE_INIT, dirPath, PROC_ERR_PATH_TOO_LONG);
        return NULL;
    }

    if (isReg(indexFilePath)) {
        errno = 0;
        FILE *indexFile = fopen(indexFilePath, "a+");
        if (!indexFile) {
            reportProcessError(
                PROCESS_OP_HANDLE_INIT, indexFilePath, PROC_ERR_OPEN_FILE);
            return NULL;
        }

        rewind(indexFile); // move to beginning of index file
        return indexFile;
    }

    errno = 0;
    FILE *indexFile = fopen(indexFilePath, "w+");
    if (!indexFile) {
        reportProcessError(
            PROCESS_OP_HANDLE_INIT, indexFilePath, PROC_ERR_OPEN_FILE);
        return NULL;
    }

    return indexFile;
}

static int isIncludedFilter(const struct dirent *entry) {
    if (!entry) {
        return 0;
    }
    return isIncluded(entry->d_name);
}

int enqueue(struct InitQueue **queue, const char *dirPath) {
    if (!queue) {
        return -1;
    }

    if (!dirPath) {
        return -1;
    }

    struct InitQueue *newNode = malloc(sizeof(struct InitQueue));
    if (!newNode) {
        return -1;
    }

    size_t dirPathLen = strlen(dirPath) + 1;
    char *pathCopy = malloc(dirPathLen);
    if (!pathCopy) {
        free(newNode);
        return -1;
    }
    memcpy(pathCopy, dirPath, dirPathLen);

    newNode->dirPath = pathCopy;
    newNode->next = NULL;

    if (!*queue) {
        *queue = newNode;
    } else {
        struct InitQueue *cur = *queue;
        while (cur->next) {
            cur = cur->next;
        }
        cur->next = newNode;
    }

    return 0;
}

int dequeue(struct InitQueue **queue, char *outPath, size_t outPathLen) {
    if (!*queue) {
        return -1;
    }

    struct InitQueue *oldHead = *queue;

    size_t pathLen = strlen(oldHead->dirPath) + 1;
    if (pathLen > outPathLen) {
        reportProcessError(
            PROCESS_OP_HANDLE_INIT, oldHead->dirPath, PROC_ERR_PATH_TOO_LONG);
        return -1;
    }
    memcpy(outPath, oldHead->dirPath, pathLen);

    *queue = oldHead->next;

    free(oldHead->dirPath);
    free(oldHead);

    return 0;
}

int handleInit(char *rootDir) {
    if (!rootDir) {
        reportProcessError(PROCESS_OP_HANDLE_INIT, "(root directory is NULL)", PROC_ERR_INVALID_PATH);
        return -1;
    }

    int errorCount = 0;
    struct InitQueue *queue = NULL;

    if (enqueue(&queue, rootDir) != 0) {
                    reportProcessError(PROCESS_OP_HANDLE_INIT, rootDir, PROC_ERR_MEMORY_ALLOC);
        return -1;
    }

    while (queue) {
        char curDir[COLETTE_PATH_BUF_SIZE];

        if (dequeue(&queue, curDir, sizeof(curDir)) != 0) {
            break;
        }

        FILE *indexFile = openOrCreateIndexFile(curDir);
        if (!indexFile) {
            reportProcessError(
                PROCESS_OP_HANDLE_INIT, curDir, PROC_ERR_INDEX_MISSING);
            errorCount++;
            continue;
        }

        struct dirent **nameList;
        int entryCount =
            scandir(curDir, &nameList, isIncludedFilter, alphasort);
        if (entryCount < 0) {
            reportProcessError(
                PROCESS_OP_HANDLE_INIT, curDir, PROC_ERR_OPEN_FILE);
            errorCount++;
            fclose(indexFile);
            continue;
        }

        for (int i = 0; i < entryCount; i++) {
            const char *entry = nameList[i]->d_name;
            char fullPath[COLETTE_PATH_BUF_SIZE];
            if (joinPath(fullPath, sizeof(fullPath), curDir, entry)) {
                errorCount++;
                free(nameList[i]);
                continue;
            }

            if (isReg(fullPath)) {
                addToIndexFile(indexFile, entry);
            } else if (isDir(fullPath)) {
                addToIndexFile(indexFile, entry);
                if (enqueue(&queue, fullPath) != 0) {
                    reportProcessError(PROCESS_OP_HANDLE_INIT, curDir, PROC_ERR_MEMORY_ALLOC);
                }
            }
            free(nameList[i]);
        }

        free(nameList);

        fclose(indexFile);
    }

    while (queue) {
        char tmpPath[COLETTE_PATH_BUF_SIZE];
        dequeue(&queue, tmpPath, sizeof(tmpPath));
    }

    return errorCount > 0 ? -1 : 0;
}
