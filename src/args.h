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
    ARG_MISSING_TITLE,
    ARG_INVALID_TITLE,
    ARG_NO_DIR_ACCESS,
    ARG_CONFLICTING_FLAGS,
    ARG_INVALID_OPT,
    ARG_MEMORY_ERROR,
    ARG_USAGE_MSG,
};

enum ProcessMode {
    MODE_COLLATE = 0,
    MODE_LIST,
    MODE_CHECK,
};

struct Arguments {
    char *directory;
    char *title;
    bool initMode;
    enum ProcessMode mode;
    unsigned int prefixPadding;
    enum ArgError status;
};

struct Arguments parseArgs(int argc, char *argv[]);
const char *argErrorToString(enum ArgError error);
const char *getUsageString(void);
void freeArguments(struct Arguments *args);

#endif
