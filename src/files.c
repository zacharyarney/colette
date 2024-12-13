#include "files.h"
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

bool isDirectory(const char *path) {
    struct stat statBuf;
    if (stat(path, &statBuf) != 0) {
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
     * a file used to keep notes not intended to be isIncluded in the document.
     * Dot-prefixed files are normal POSIX hidden files and include `.index`
     * files to be used to organize the document. Finally, output file names
     * will be surrounded by underscores i.e. `_Title_.md` to both filter them
     * from collation and visually differentiate them from other project files.
     **/
    return (fileName[0] != '.' && fileName[0] != '_');
}

char *getBasename(const char *path, char *buffer, size_t size) {
    if (!path || !buffer || size == 0) {
        return NULL;
    }
    return "wow";
}

int collateFile(const char *path, FILE *outFileP) {
    FILE *inFileP = fopen(path, "r");
    char buffer[8192];
    size_t bytesRead;

    if (!inFileP) {
        fprintf(stderr, "Error opening %s: %s\n", path, strerror(errno));
        return EXIT_FAILURE;
    }

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), inFileP) > 0)) {
        size_t bytesWritten = fwrite(buffer, 1, sizeof(buffer), outFileP);
        if (bytesWritten != bytesRead) {
            fprintf(stderr, "Error writing %s to output: %s\n", path,
                    strerror(errno));
            return EXIT_FAILURE;
        }
    }

    if (ferror(inFileP)) {
        fprintf(stderr, "Error reading from %s: %s\n", path, strerror(errno));
    }

    fprintf(outFileP, "\n");
    fclose(inFileP);

    return EXIT_SUCCESS;
}

// int readIndexFile(const char *path) {
//     char basename[NAME_MAX + 1];
// 
//     return EXIT_SUCCESS;
// }
