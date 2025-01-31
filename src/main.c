#include "args.h"
#include "process.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    struct Arguments args = parseArgs(argc, argv);
    if (args.status != ARG_SUCCESS) {
        return EXIT_FAILURE;
    }

    int projectSuccess = processProject(&args);

    // DON'T FORGET TO FREE
    freeArguments(&args);

    if (projectSuccess != 0) {
        printf("PROJECT FAILED: %d\n", projectSuccess);
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Success");
    return EXIT_SUCCESS;
}
