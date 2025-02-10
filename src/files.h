#ifndef FILES_H
#define FILES_H

#include "errors.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

bool isIncluded(const char *fileName);
// int getFileStatus(struct stat *statBuf, const char *path);
enum ResolveStatus resolveFile(char *buffer, size_t buffSize, const char *path);
int getBasename(char *buffer, const char *path, size_t size);
int handlePathBufTrailingSlashPad(const char *path, size_t pathLen);
int joinPath(char *buffer, size_t buffSize, const char *dir, const char *file);
int joinExtension(char *buffer,
                  size_t buffSize,
                  const char *file,
                  const char *ext);
#endif /* FILES_H */
