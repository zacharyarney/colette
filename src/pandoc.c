#include "pandoc.h"
#include <stdio.h>
#include <stdlib.h>

int checkPandoc(void) {
    int installed = system("pandoc --version > /dev/null 2>&1");
    if (installed != 0) {
        fprintf(stderr, "Error: pandoc is not installed.\n");
        fprintf(stderr, "Please install pandoc from https://pandoc.org");
    }
    return installed;
}
