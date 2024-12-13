#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

enum ArgError {
    ARG_SUCCESS = 0,
    ARG_MISSING_DIR,
    ARG_INVALID_DIR,
    ARG_MISSING_PADDING,
    ARG_INVALID_PADDING,
    ARG_PADDING_RANGE,
    ARG_NO_DIR_ACCESS,
    ARG_CONFLICTING_FLAGS,
    ARG_INVALID_OPT,
    ARG_MEMORY_ERROR
};

struct Arguments {
    char *directory;
    bool initMode;
    bool checkMode;
    bool outputDirMode;
    unsigned int prefixPadding;
};

enum ParseError { PARSE_SUCCESS = 0, PARSE_ERROR };

enum ArgError parseArgs(int argc, char *argv[], struct Arguments *args);
const char *argErrorToString(enum ArgError error);
void freeArguments(struct Arguments *args);

#endif
