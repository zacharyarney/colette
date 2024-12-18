#include "args.h"
#include "constants.h"
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct option longOpts[] = {
    {"init", no_argument, NULL, 'i'},
    {"check", no_argument, NULL, 'c'},
    {"as-list", no_argument, NULL, 'l'},
    {"title", required_argument, NULL, 't'},
    {"prefix", required_argument, NULL, 'p'},
    // {"output", required_argument, NULL, 'o'},
    {0, 0, 0, 0} // array terminator
};

static bool isValidFilenameChar(char c) {
    if (c == '/' || c == '\0' || iscntrl(c)) {
        return false;
    }

    return true;
}

static unsigned int stringToUint(const char *str, char **endptr,
                                 bool *success) {
    if (!str || !success) {
        if (success) {
            *success = false;
        }
        return 0;
    }

    errno = 0;
    unsigned long value = strtoul(str, endptr, 10);
    if (errno == ERANGE || value > UINT_MAX) {
        *success = false;
        return 0;
    }

    *success = true;

    return (unsigned int)value;
}

static char *validateDirectory(char *directoryArg, enum ArgError *status) {
    char *resolvedPath;
    char *directory = directoryArg ? directoryArg : ".";

    // null pointer in resolved_name arg mallocs buffer for return value
    // rememeber to free()!
    resolvedPath = realpath(directory, NULL);
    if (resolvedPath == NULL) {
        *status = ARG_INVALID_DIR;
        switch (errno) {
        case EACCES:
            *status = ARG_NO_DIR_ACCESS;
            break;
        case ENOENT:
            *status = ARG_INVALID_DIR;
            break;
        default:
            fprintf(stderr, "Error resolving path %s: %s\n", directory,
                    strerror(errno));
            *status = ARG_INVALID_DIR;
        }
        return NULL;
    }

    *status = ARG_SUCCESS;

    return resolvedPath;
}

static void validateModes(struct Arguments *args, enum ArgError *status) {
    if (args->initMode) {
        if (args->checkMode || args->outputDirMode) {
            *status = ARG_CONFLICTING_FLAGS;
        }
    } else if (args->checkMode && args->outputDirMode) {
        *status = ARG_CONFLICTING_FLAGS;
    }
}

static unsigned int validatePadding(char *paddingArg, enum ArgError *status) {
    char *endptr;
    bool success;
    unsigned int padding = stringToUint(paddingArg, &endptr, &success);
    if (!success || *endptr != '\0') {
        *status = ARG_INVALID_PADDING;
    } else if (padding <= 0 || padding > 10) {
        *status = ARG_PADDING_RANGE;
    } else {
        *status = ARG_SUCCESS;
    }

    return padding;
}

static char *validateTitle(char *titleArg, enum ArgError *status) {
    if (!titleArg || titleArg[0] == '\0') {
        *status = ARG_MISSING_TITLE;
        return NULL;
    }

    size_t titleLen = strlen(titleArg);
    size_t outLen = titleLen + 3; // title + underscores + null terminator

    if (outLen > COLETTE_NAME_BUF_SIZE) {
        *status = ARG_INVALID_TITLE;
        return NULL;
    }

    char *titleBuf = malloc(outLen);
    if (!titleBuf) {
        *status = ARG_MEMORY_ERROR;
        return NULL;
    }

    // build output title
    titleBuf[0] = '_';
    memcpy(titleBuf + 1, titleArg, titleLen);
    titleBuf[titleLen + 1] = '_';
    titleBuf[titleLen + 2] = '\0';

    for (size_t i = 1; i < outLen - 1; i++) {
        if (!isValidFilenameChar(titleBuf[i])) {
            *status = ARG_INVALID_TITLE;
            return NULL;
        }
        if (isspace(titleBuf[i])) {
            titleBuf[i] = '_';
        }
    }

    *status = ARG_SUCCESS;
    return titleBuf;
}

enum ArgError parseArgs(int argc, char **argv, struct Arguments *args) {
    int opt;
    char *shortOpts = "cilt:p:";
    enum ArgError status = ARG_SUCCESS;

    while ((opt = getopt_long(argc, argv, shortOpts, longOpts, NULL)) != -1) {
        switch (opt) {
        case 'i':
            args->initMode = true;
            break;
        case 'c':
            args->checkMode = true;
            break;
        case 'o':
            args->outputDirMode = true;
            break;
        case 't':
            args->title = validateTitle(optarg, &status);
            break;
        case 'p':
            args->prefixPadding = validatePadding(optarg, &status);
            if (status != ARG_SUCCESS) {
                return status;
            }
            break;
        case '?':
            return ARG_INVALID_OPT;
        }
    }

    // Set default title if not supplied by user
    if (!args->title) {
        args->title = "_draft_";
    }
    // After optional args are parsed, optint points to first non-optional arg.
    // This allows us to set args->directory to DIRECTORY.
    args->directory = validateDirectory(argv[optind], &status);
    validateModes(args, &status);

    return status;
}

const char *argErrorToString(enum ArgError error) {
    switch (error) {
    case ARG_SUCCESS:
        return "Success";
    case ARG_MISSING_DIR:
        return "Error: No directory specified";
    case ARG_INVALID_DIR:
        return "Error: Invalid directory path";
    case ARG_MISSING_PADDING:
        return "Error: Prefix padding value required";
    case ARG_INVALID_PADDING:
        return "Error: Invalid prefix padding value";
    case ARG_PADDING_RANGE:
        return "Error: Prefix padding must be a value from 1 to 10";
    case ARG_MISSING_TITLE:
        return "Error: Title value required";
    case ARG_INVALID_TITLE:
        return "Error: Invalid title";
    case ARG_NO_DIR_ACCESS:
        return "Error: Cannot access directory";
    case ARG_CONFLICTING_FLAGS:
        return "Error: Conflicting options specified";
    case ARG_INVALID_OPT:
        return "Error: Invalid option";
    case ARG_MEMORY_ERROR:
        return "Error: Memory allocation failed";
    default:
        return "Unknown error";
    }
}

void freeArguments(struct Arguments *args) {
    if (args->directory != NULL) {
        free(args->directory);
    }
}
