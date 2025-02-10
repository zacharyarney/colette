#ifndef CONSTANTS_H
#define CONSTANTS_H

/* *
 * Common system page size. Should be large enough buffer for most paths
 * */
#define COLETTE_PATH_BUF_SIZE 4096
#define COLETTE_MAX_PATH_LEN (COLETTE_PATH_BUF_SIZE - 1)

/* *
 * Common system filename size
 * */
#define COLETTE_NAME_BUF_SIZE 255

/* *
 * Reasonable buffer size for file name extension
 * */
#define COLETTE_EXT_BUF_SIZE 16

/* *
 * Size of read buffer for file collation
 * */
#define COLETTE_FILE_BUF_SIZE 8192

/* *
 * Initial project depth value allows for 5 layers of nesting.
 * */
#define COLETTE_PROJECT_DEPTH 5
#endif
