#include "reporting.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char *fileOpStr(enum FileOperation op) {
    switch (op) {
    case FILE_OP_CHECK:
        return "checking";
    case FILE_OP_ACCESS:
        return "accessing";
    case FILE_OP_OPEN:
        return "opening";
    case FILE_OP_READ:
        return "reading";
    case FILE_OP_WRITE:
        return "writing";
    case PATH_OP_VALIDATE:
        return "validating path inputs";
    case PATH_OP_COMPONENT:
        return "validating path component";
    case PATH_OP_ABS_FILE:
        return "validating path component (absolute path in index file)";
    case PATH_OP_BUFFER:
        return "managing path buffer";
    case PATH_OP_JOIN:
        return "joining paths";
    default:
        return "performing unknown operation on";
    }
}

static const char *processErrorStr(enum ProcessErrorDetail detail) {
    switch (detail) {
    // Memory and resource errors
    case PROC_ERR_MEMORY_ALLOC:
        return "Failed to allocate required memory";
    case PROC_ERR_RESOURCE_EXHAUSTED:
        return "System resources exhausted";
    case PROC_ERR_STACK_OVERFLOW:
        return "Project structure too deeply nested";

    // Path and name errors
    case PROC_ERR_PATH_TOO_LONG:
        return "Path exceeds maximum allowed length";
    case PROC_ERR_NAME_INVALID:
        return "Name contains invalid characters";
    case PROC_ERR_NAME_TOO_LONG:
        return "Name component too long";
    case PROC_ERR_FILE_NOT_FOUND:
        return "File not found";
    case PROC_ERR_INVALID_PATH:
        return "Invalid path";
    case PROC_ERR_INVALID_LINK:
        return "Symbolic links not permitted";

    // Structure errors
    case PROC_ERR_INDEX_MISSING:
        return "Required .index file not found";
    case PROC_ERR_INDEX_FORMAT:
        return "Invalid .index file format";
    case PROC_ERR_INDEX_EMPTY:
        return "Index file is empty";
    case PROC_ERR_CIRCULAR_REF:
        return "Circular reference detected in project structure";

    // Operation errors
    case PROC_ERR_INVALID_MODE:
        return "Invalid or unsupported processing mode";
    case PROC_ERR_INVALID_STATE:
        return "Invalid internal program state";
    case PROC_ERR_INVALID_SEQUENCE:
        return "Operations performed in invalid sequence";

    // Permission errors
    case PROC_ERR_ACCESS_DENIED:
        return "Permission denied for operation";
    case PROC_ERR_LOCKED:
        return "Required resource is locked";

    // Data errors
    case PROC_ERR_BUFFER_OVERFLOW:
        return "Internal buffer overflow";
    case PROC_ERR_DATA_CORRUPT:
        return "Data corruption detected";

    // Project structure errors
    case PROC_ERR_TOO_DEEP:
        return "Project hierarchy exceeds maximum depth";
    case PROC_ERR_TOO_MANY_FILES:
        return "Project contains too many files";
    case PROC_ERR_INVALID_STRUCTURE:
        return "Invalid project structure detected";

    // Other
    case PROC_ERR_OPEN_FILE:
        return "Unable to open file";

    default:
        return "Unknown error occurred";
    }
}

static const char *processOpStr(enum ProcessOperation op) {
    switch (op) {
    // Iterator operations
    case PROCESS_OP_ITER_INIT:
        return "initializing file iterator";
    case PROCESS_OP_ITER_PUSH:
        return "adding directory to processing queue";
    case PROCESS_OP_ITER_POP:
        return "removing directory from processing queue";
    case PROCESS_OP_ITER_NEXT:
        return "getting next file";

    // Context operations
    case PROCESS_OP_CTX_INIT:
        return "initializing process context";
    case PROCESS_OP_CTX_PATH:
        return "setting file path";
    case PROCESS_OP_CTX_OUTPUT:
        return "setting output location";

    // State operations
    case PROCESS_OP_STATE_INIT:
        return "initializing project state";
    case PROCESS_OP_STATE_MODE:
        return "setting processing mode";
    case PROCESS_OP_STATE_CLEANUP:
        return "cleaning up resources";

    // Handler operations
    case PROCESS_OP_HANDLE_CHECK:
        return "checking project structure";
    case PROCESS_OP_HANDLE_INIT:
        return "initializing project";
    case PROCESS_OP_HANDLE_LIST:
        return "creating file listing";
    case PROCESS_OP_HANDLE_COLLATE:
        return "combining files";

    default:
        return "unknown operation";
    }
}

void reportProcessError(enum ProcessOperation op,
                        const char *path,
                        enum ProcessErrorDetail detail) {
    fprintf(stderr, "Error %s", processOpStr(op));

    if (path) {
        fprintf(stderr, " for %s", path);
    }

    const char *detailStr = processErrorStr(detail);
    if (detailStr) {
        fprintf(stderr, ": %s", detailStr);
    }

    if (errno != 0) {
        fprintf(stderr, " (%s)", strerror(errno));
    }

    fprintf(stderr, "\n");
}

void reportFileError(enum FileOperation op, const char *path) {
    fprintf(stderr, "Error %s %s", fileOpStr(op), path ? path : "path");
    if (errno != 0) {
        fprintf(stderr, ": %s", strerror(errno));
    }
    fprintf(stderr, "\n");
}
