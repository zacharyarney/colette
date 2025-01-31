#include "constants.h"
#include "errors.h"
#include "files.h"
#include "reporting.h"
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *SUPPORTED_FILE_EXTENSIONS[] = {
    ".md",
    ".txt",
    NULL,
};

static const char **getSupportedExtensions(void) {
    return SUPPORTED_FILE_EXTENSIONS;
}

// static size_t getNumSupportedExtensions(void) {
//     size_t count = 0;
//     while (SUPPORTED_FILE_EXTENSIONS[count] != NULL) {
//         count++;
//     }
//     return count;
// }

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

enum ResolveStatus
resolveFile(char *buffer, size_t buffSize, const char *path) {
    if (!path || path[0] == '\0') {
        return RESOLVE_ERROR;
    }

    size_t pathLen = strlen(path) + 1;
    struct stat statBuf;
    if (lstat(path, &statBuf) == 0) {
        if (S_ISLNK(statBuf.st_mode)) {
            return RESOLVE_LINK;
        }
        if (S_ISDIR(statBuf.st_mode)) {
            memcpy(buffer, path, pathLen);
            return RESOLVE_DIR;
        }
        if (S_ISREG(statBuf.st_mode)) {
            memcpy(buffer, path, pathLen);
            return RESOLVE_EXACT;
        }

        return RESOLVE_ERROR;
    }

    const char **extensions = getSupportedExtensions();
    while (*extensions != NULL) {
        if (joinExtension(buffer, buffSize, path, *extensions) !=
            PATH_SUCCESS) {
            return RESOLVE_ERROR;
        }

        if (lstat(buffer, &statBuf) == 0) {
            if (S_ISLNK(statBuf.st_mode)) {
                return RESOLVE_LINK;
            }

            if (S_ISREG(statBuf.st_mode)) {
                return RESOLVE_FILE;
            }
        }

        extensions++;
    }

    return RESOLVE_NOT_FOUND;
}

int getBasename(const char *path, char *buffer, size_t size) {
    if (!path || !buffer || size == 0) {
        return -1;
    }

    size_t len = strlen(path) + 1;
    char *pathCopy = malloc(len);
    if (!pathCopy) {
        reportFileError(FILE_OP_ACCESS, path); // memory error accessing path
        return -1;
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

int handlePathBufTrailingSlashPad(const char *path, size_t pathLen) {
    bool hasTrailingSlash = (path[pathLen - 1] == '/');
    return hasTrailingSlash ? 1 : 2; // slash and null terminator
}

enum PathStatus
joinPath(char *buffer, size_t buffSize, const char *dir, const char *file) {
    if (!dir || !file || !buffer) {
        reportFileError(PATH_OP_VALIDATE, "path joining");
        return PATH_NULL_INPUT;
    }
    if (buffSize > COLETTE_PATH_BUF_SIZE) {
        reportFileError(PATH_OP_BUFFER, dir);
        return PATH_BUFFER_TOO_LARGE;
    }
    if (dir[0] == '\0' || file[0] == '\0') {
        reportFileError(PATH_OP_VALIDATE, "path joining");
        return PATH_EMPTY_INPUT;
    }

    size_t dirLen = strnlen(dir, buffSize);
    size_t fileLen = strnlen(file, buffSize);
    if (dirLen >= buffSize || fileLen >= buffSize) {
        // strnlen did not encounter null terminator
        reportFileError(PATH_OP_VALIDATE, "path joining");
        return PATH_TOO_LONG;
    }

    if (file[0] == '/') {
        fprintf(stderr, "DEBUG -- file starts with a slash (absolute)");
        reportFileError(PATH_OP_ABS_FILE, file);
        return PATH_ABS_NOT_ALLOWED;
    }
    if (fileLen > COLETTE_NAME_BUF_SIZE) {
        reportFileError(PATH_OP_COMPONENT, file);
        return PATH_NAME_TOO_LONG;
    }

    int extraChars = handlePathBufTrailingSlashPad(dir, dirLen);

    if (dirLen + fileLen + extraChars > COLETTE_MAX_PATH_LEN) {
        reportFileError(PATH_OP_JOIN, "path joining");
        return PATH_TOO_LONG;
    }

    memcpy(buffer, dir, dirLen);

    if (extraChars == 2) {
        buffer[dirLen] = '/';
        dirLen++;
    }

    memcpy(buffer + dirLen, file, fileLen);
    buffer[dirLen + fileLen] = '\0';

    return PATH_SUCCESS;
}

enum PathStatus joinExtension(char *buffer,
                              size_t buffSize,
                              const char *file,
                              const char *ext) {
    if (!file || !ext || !buffer) {
        reportFileError(FILE_OP_ACCESS, "extension joining");
        return PATH_NULL_INPUT;
    }
    if (buffSize > COLETTE_PATH_BUF_SIZE) {
        reportFileError(FILE_OP_ACCESS, file);
        return PATH_BUFFER_TOO_LARGE;
    }
    if (file[0] == '\0' || ext[0] == '\0') {
        reportFileError(FILE_OP_ACCESS, "extension joining");
        return PATH_EMPTY_INPUT;
    }
    if (ext[0] != '.') {
        reportFileError(FILE_OP_ACCESS, "extension joining");
        return PATH_INVALID_EXT;
    }

    size_t fileLen = strnlen(file, buffSize);
    size_t extLen = strnlen(ext, COLETTE_NAME_BUF_SIZE) + 1;
    if (extLen >= COLETTE_NAME_BUF_SIZE || fileLen >= buffSize) {
        // strnlen did not encounter null terminator
        reportFileError(FILE_OP_ACCESS, "extension joining");
        return PATH_TOO_LONG;
    }

    if (fileLen + extLen > COLETTE_MAX_PATH_LEN) {
        reportFileError(FILE_OP_ACCESS, "extension joining");
        return PATH_TOO_LONG;
    }

    memcpy(buffer, file, fileLen);
    memcpy(buffer + fileLen, ext, extLen); // copy ext at end of file in buffer

    return PATH_SUCCESS;
}
