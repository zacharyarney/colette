#ifndef REPORTING_H
#define REPORTING_H

#include "errors.h"

/* *
 * Reports errors specific to file operations to stderr. Includes system errno
 * message when available.
 *
 * @param  op    Type of file operation that failed
 * @param  path  Path to file that caused error, or descriptive string if no path
 * */
void reportFileError(enum FileOperation op, const char *path);

/* *
 * Reports a process error to stderr with detailed context. Includes operation
 * type, relevant path, and specific error details. Also includes system errno
 * message when available.
 *
 * @param  op       Type of process operation that failed
 * @param  path     Path related to error, or NULL if no path involved
 * @param  details  Specific error detail code describing what went wrong
 * */
void reportProcessError(enum ProcessOperation op,
                        const char *path,
                        enum ProcessErrorDetail details);

#endif
