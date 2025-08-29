#ifndef FILES_H
#define FILES_H

#include "errors.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

/* *
 * Determines if a file or directory is to be included in the final output.
 *
 * Hidden files are denoted with a '.' or '_' prefix. User-created files are to
 * be prefixed with '_' to be ignored. For example: `_notes.md` could be a file
 * used to keep notes not intended to be included in the document. Dot-prefixed
 * files are normal POSIX hidden files and include `.index` files to be used to
 * organize the document. Finally, output file names will be surrounded by
 * underscores i.e. `_Title_.md` to both filter them from collation and
 * visually differentiate them from other project files.
 *
 * @param   fileName  Name of file or directory to be checked
 *
 * @return  bool
 *          true      if file should be included in finale output
 *          false     if file should be ignored
 * */
bool isIncluded(const char *fileName);

/* *
 * Determines if a path is a directory.
 *
 * @param   path    Name of path to be checked
 *
 * @return  bool
 *          true    if path is a directory
 *          false   if path is not a directory
 * */
bool isDir(const char *path);

/* *
 * Determines if a path is a regular file.
 *
 * @param   path    Name of path to be checked
 *
 * @return  bool
 *          true    if path is a directory
 *          false   if path is not a directory
 * */
bool isReg(const char *path);

/* *
 * Distinguishes between directories and regular files and symbolic links.
 * Buffer must be large enough to hold the path and longest supported extension.
 *
 * @param   buffer             Output buffer to store resolved path
 * @param   buffSize           Size of output buffer
 * @param   path               Path to resolve
 *
 * @return  ResolveStatus      indicating result:
 *          RESOLVE_ERROR      General error occurred
 *          RESOLVE_NO_ACCESS  Permission denied
 *          RESOLVE_NOT_FOUND  No matching file found
 *          RESOLVE_LINK       Found symbolic link (not allowed)
 *          RESOLVE_DIR        Found a directory
 *          RESOLVE_FILE       Found file with added extension
 *          RESOLVE_EXACT      Found exact match
 * */
enum ResolveStatus resolveFile(char *buffer, size_t buffSize, const char *path);

/* *
 * Wrapper around POSIX basename() with added buffer safety. Extracts the
 * basename (filename) from a path string. Handles trailing slashes.
 *
 * @param   buffer  Output buffer for basename
 * @param   path    Full path to extract basename from
 * @param   size    Size of output buffer
 *
 * @return  int
 *          0       on success
 *         -1       on error
 *
 * */
int getBasename(char *buffer, const char *path, size_t size);

/* *
 * Calculates padding needed for path joining with trailing slash handling.
 * Used to ensure proper buffer allocation for path construction.
 *
 * @param   path     Path to analyze
 * @param   pathLen  Length of the path
 *
 * @return  int
 *          1        if path has trailing slash
 *          2        if path doesn't have trailing slash
 *         (accounts for added slash and null terminator)
 * */
int handlePathBufTrailingSlashPad(const char *path, size_t pathLen);

/* *
 * Safely joins directory and file paths, preventing directory traversal.
 * Ensures resulting path stays within project boundaries. Handles missing
 * trailing slashes, prevents directory traversal, validates buffer sizes and
 * path components.
 *
 * @param   buffer    Output buffer for joined path
 * @param   buffSize  Size of output buffer
 * @param   dir       Directory path
 * @param   file      File path to append (must not be absolute)
 *
 * @return  int
 *          0         on success
 *         -1         on error
 *
 * */
int joinPath(char *buffer, size_t buffSize, const char *dir, const char *file);

/* *
 * Safely appends a file extension to a path. Validates both the base path and
 * extension. Validates extension format, handles buffer sizes, ensures
 * resulting filename is valid.
 *
 * @param   buffer    Output buffer for path with extension
 * @param   buffSize  Size of output buffer
 * @param   file      Base file path
 * @param   ext       Extension to append (must start with '.')
 *
 * @return  int
 *          0         on success
 *         -1         on error
 *
 * */
int joinExtension(char *buffer,
                  size_t buffSize,
                  const char *file,
                  const char *ext);
#endif /* FILES_H */
