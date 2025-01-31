#ifndef FILES_H
#define FILES_H

#include "errors.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

bool isIncluded(const char *fileName);
// int getFileStatus(struct stat *statBuf, const char *path);
enum ResolveStatus resolveFile(char *buffer, size_t buffSize, const char *path);
int getBasename(const char *path, char *buffer, size_t size);
int handlePathBufTrailingSlashPad(const char *path, size_t pathLen);
enum PathStatus
joinPath(char *buffer, size_t buffSize, const char *dir, const char *file);
enum PathStatus
joinExtension(char *buffer, size_t buffSize, const char *file, const char *ext);
#endif /* FILES_H */
