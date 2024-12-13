#include "args.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    struct Arguments args = {.directory = NULL,
                             .initMode = false,
                             .checkMode = false,
                             .outputDirMode = false,
                             .prefixPadding = 3};

    enum ArgError argParseStatus = parseArgs(argc, argv, &args);
    if (argParseStatus != ARG_SUCCESS) {
        fprintf(stderr, "%s\n", argErrorToString(argParseStatus));
        fprintf(stderr,
                "Usage: colette [-i|--init] [-c|--check] [-a|--as-list] "
                "[-p|--prefix] PREFIX_PADDING DIRECTORY\n");
        return EXIT_FAILURE;
    } else {
        printf("%s\n", argErrorToString(argParseStatus));
    }

    // DON'T FORGET TO FREE
    freeArguments(&args);

    return EXIT_SUCCESS;
}
