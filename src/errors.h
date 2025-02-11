#ifndef ERRORS_H
#define ERRORS_H

enum FileOperation {
    FILE_OP_CHECK,     // Checking file properties (type, existence)
    FILE_OP_ACCESS,    // Permission/access issues
    FILE_OP_OPEN,      // Opening files (will need this for index files)
    FILE_OP_READ,      // Reading from files (will need this for collation)
    FILE_OP_WRITE,     // Writing to files (will need this for output)
    FILE_OP_JOIN,
    PATH_OP_VALIDATE,  // Input validation checks
    PATH_OP_COMPONENT, // Path component validation
    PATH_OP_ABS_FILE,
    PATH_OP_BUFFER,    // Buffer management
    PATH_OP_JOIN       // Path joining operation
};

enum ResolveStatus {
    RESOLVE_ERROR,     // Something went wrong during resolution
    RESOLVE_NO_ACCESS, // Permission/access issues
    RESOLVE_NOT_FOUND, // No matching file found
    RESOLVE_LINK,      // Found symbolic link -- not allowed
    RESOLVE_DIR,       // Found a directory
    RESOLVE_FILE,      // Found a file with extension
    RESOLVE_EXACT,     // Found exact match
};

enum ProcessOperation {
    // Iterator operations
    PROCESS_OP_ITER_INIT, // Failed to initialize the file iterator
    PROCESS_OP_ITER_PUSH, // Failed to push new index state onto stack
    PROCESS_OP_ITER_POP,  // Failed to pop index state from stack
    PROCESS_OP_ITER_NEXT, // Failed to get next file

    // Context operations
    PROCESS_OP_CTX_INIT,   // Failed to initialize process context
    PROCESS_OP_CTX_PATH,   // Failed to set current file path
    PROCESS_OP_CTX_OUTPUT, // Failed to set/create output path

    // State operations
    PROCESS_OP_STATE_INIT,    // Failed to initialize project state
    PROCESS_OP_STATE_MODE,    // Failed to set processing mode
    PROCESS_OP_STATE_CLEANUP, // Failed to clean up resources

    // Handler operations
    PROCESS_OP_HANDLE_CHECK,   // Failed during structure check
    PROCESS_OP_HANDLE_INIT,    // Failed during project initialization
    PROCESS_OP_HANDLE_LIST,    // Failed during symlink creation
    PROCESS_OP_HANDLE_COLLATE, // Failed during file collation
};

enum ProcessErrorDetail {
    // Memory and resource errors
    PROC_ERR_MEMORY_ALLOC,       // Generic memory allocation failure
    PROC_ERR_RESOURCE_EXHAUSTED, // Ran out of system resources
    PROC_ERR_STACK_OVERFLOW,     // Project nesting too deep

    // Path and name errors
    PROC_ERR_PATH_TOO_LONG, // Path exceeds system limits
    PROC_ERR_NAME_INVALID,  // Invalid characters in name
    PROC_ERR_NAME_TOO_LONG, // Name component too long

    // Structure errors
    PROC_ERR_INDEX_MISSING, // No .index file found
    PROC_ERR_INDEX_FORMAT,  // Malformed .index file
    PROC_ERR_INDEX_EMPTY,   // Empty .index file
    PROC_ERR_CIRCULAR_REF,  // Circular reference detected

    // Operation errors
    PROC_ERR_INVALID_MODE,     // Invalid processing mode
    PROC_ERR_INVALID_STATE,    // Invalid internal state
    PROC_ERR_INVALID_SEQUENCE, // Operations in wrong order
    PROC_ERR_INVALID_LINK,     // Symbolic links not allowed
    PROC_ERR_INVALID_PATH,
    PROC_ERR_INVALID_OUTPUT,
    PROC_ERR_FILE_NOT_FOUND,

    // Permission errors
    PROC_ERR_ACCESS_DENIED, // Permission denied
    PROC_ERR_LOCKED,        // Resource is locked

    // Data errors
    PROC_ERR_BUFFER_OVERFLOW, // Internal buffer overflow
    PROC_ERR_DATA_CORRUPT,    // Data corruption detected

    // Project structure errors
    PROC_ERR_TOO_DEEP,          // Project hierarchy too deep
    PROC_ERR_TOO_MANY_FILES,    // Too many files in project
    PROC_ERR_INVALID_STRUCTURE, // Invalid project structure

    // Other
    PROC_ERR_OPEN_FILE
};

enum FileIteratorStatus {
    ITER_SUCCESS,
    ITER_IGNORE,
    ITER_FAILURE,
    ITER_END,
};

enum FileHandlerStatus {
    HANDLER_SUCCESS,
    HANDLER_FAILURE,
};

enum ProcessContextStatus {
    CTX_SUCCESS,
    CTX_FAILURE,
};

enum ProjectStateStatus {
    STATE_SUCCESS,
    STATE_FAILURE,
};

#endif
