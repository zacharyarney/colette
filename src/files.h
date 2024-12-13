#ifndef FILES_H
#define FILES_H

#include <stdbool.h>
#include <stdio.h>

bool isDirectory(const char *path);
bool isIncluded(const char *fileName);
int getBasename(const char *path, char *buffer, size_t size);
int collateFile(const char *path, FILE *outFile);
int readIndexFile(const char *path);

#endif /* FILES_H */
