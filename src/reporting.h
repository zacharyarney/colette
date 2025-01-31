#ifndef REPORTING_H
#define REPORTING_H

#include "errors.h"

void reportFileError(enum FileOperation op, const char *path);
void reportProcessError(enum ProcessOperation op,
                        const char *path,
                        enum ProcessErrorDetail details);

#endif
