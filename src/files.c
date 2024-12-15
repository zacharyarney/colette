#include "constants.h"
#include "files.h"
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

bool isDirectory(const char *path) {
    struct stat statBuf;
    if (stat(path, &statBuf) != 0) {
        if (errno != ENOENT) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
        }
        return false;
    }
    return S_ISDIR(statBuf.st_mode);
}

bool isIncluded(const char *fileName) {
    if (fileName == NULL || fileName[0] == '\0') {
        return false;
    }
    /**
     * Hidden files are denoted with a '.' or '_' prefix. User-created files
     * are prefixed with '_' to be ignored. For example: `_notes.md` could be
     * a file used to keep notes not intended to be included in the document.
     * Dot-prefixed files are normal POSIX hidden files and include `.index`
     * files to be used to organize the document. Finally, output file names
     * will be surrounded by underscores i.e. `_Title_.md` to both filter them
     * from collation and visually differentiate them from other project files.
     **/
    return (fileName[0] != '.' && fileName[0] != '_');
}

int getBasename(const char *path, char *buffer, size_t size) {
    if (!path || !buffer || size == 0) {
        return -1;
    }

    size_t len = strlen(path) + 1;
    char *pathCopy = malloc(len);
    if (!pathCopy) {
        return -1; // memory allocation failure
    }
    memcpy(pathCopy, path, len);

    char *base = basename(pathCopy);
    size_t baseLen = strlen(base) + 1;
    if (baseLen > size) {
        free(pathCopy);
        return -1;
    }
    memcpy(buffer, base, baseLen);

    free(pathCopy);
    return 0;
}

int joinPath(char *buffer, size_t buffSize, const char *dir, const char *file) {
    if (!dir || !file || !buffer) {
        return -1;
    }
    if (buffSize < COLETTE_PATH_BUF_SIZE) {
        return -1;
    }
    if (file[0] == '/') {
        return -1;
    }

    size_t dirLen = strlen(dir);
    size_t fileLen = strlen(file);

    if (dirLen == 0 || fileLen == 0) {
        return -1;
    }
    if (fileLen > FILENAME_MAX) {
        return -1;
    }

    // are we in the root directory of the computer? ("/")
    bool isRoot = (memcmp(dir, "/", 2) == 0);
    int extraChars = isRoot ? 1 : 2; // slash and null terminator

    if (dirLen + fileLen + extraChars > buffSize) {
        return -1;
    }

    memcpy(buffer, dir, dirLen);

    // add trailing slash to path if not in root directory
    if (!isRoot) {
        buffer[dirLen] = '/';
        dirLen++;
    }

    memcpy(buffer + dirLen, file, fileLen);
    buffer[dirLen + fileLen] = '\0';

    return 0;
}

int collateFile(const char *path, FILE *outFileP) {
    FILE *inFileP = fopen(path, "r");
    char buffer[8192];
    size_t bytesRead;

    if (!inFileP) {
        fprintf(stderr, "Error opening %s: %s\n", path, strerror(errno));
        return -1;
    }

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), inFileP)) > 0) {
        size_t bytesWritten = fwrite(buffer, 1, sizeof(buffer), outFileP);
        if (bytesWritten != bytesRead) {
            fprintf(stderr, "Error writing %s to output: %s\n", path,
                    strerror(errno));
            return -1;
        }
    }

    if (ferror(inFileP)) {
        fprintf(stderr, "Error reading from %s: %s\n", path, strerror(errno));
    }

    fprintf(outFileP, "\n");
    fclose(inFileP);

    return 0;
}

int readIndexFile(const char *path) {
    char basename[NAME_MAX + 1];
    if (getBasename(path, basename, sizeof(basename)) != 0) {
        return -1;
    }
    if (basename[0] == '_') {
        return 0; // ignore underscore-prefixed directories
    }
    return 0;
}
