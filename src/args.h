#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

/**
 * Possible error states when parsing command line arguments.
 */
enum ArgError {
    ARG_SUCCESS,              // Arguments parsed successfully
    ARG_MISSING_DIR,          // No directory argument provided
    ARG_INVALID_DIR,          // Root directory path is invalid
    ARG_MISSING_PADDING,      // No padding value provided with -p flag
    ARG_INVALID_PADDING,      // Padding value is not a valid number
    ARG_PADDING_RANGE,        // Padding value outside allowed range (1-10)
    ARG_MISSING_TITLE,        // No title provided with -t flag
    ARG_INVALID_TITLE,        // Title contains invalid characters
    ARG_NO_DIR_ACCESS,        // Cannot access specified directory
    ARG_CONFLICTING_FLAGS,    // Incompatible flags used together
    ARG_INVALID_OPT,          // Unknown option flag provided
    ARG_MEMORY_ERROR,         // Memory allocation failed
    ARG_USAGE_MSG,            // Show usage message
};

/**
 * Operating modes that determine if and how files are processed.
 */
enum ProcessMode {
    MODE_COLLATE,
    MODE_LIST,
    MODE_CHECK,
};

/* *
 * Arguments struct holds parsed and validated command line arguments for use
 * in processProject to initialize state and determine behavior.
 * */
struct Arguments {
    char *directory;             // Path to project root directory
    char *title;                 // Name of output file or directory
    bool initMode;               // --init flag used
    enum ProcessMode mode;       // check, collate, list
    unsigned int prefixPadding;  // number of digits in output numeric prefix
    enum ArgError status;        // status of parsing for error reporting
};

/* *
 * Parses command line arguments into an Arguments structure. Validates all
 * arguments and sets appropriate error status.
 *
 * @param   argc       Number of command line arguments
 * @param   argv       Array of command line argument strings
 * @return  Arguments  structure containing parsed values
 *
 * Note: Caller must call freeArguments() on returned structure
 * */
struct Arguments parseArgs(int argc, char *argv[]);

/* *
 * Frees memory allocated within an Arguments structure.
 *
 * @param  args  Pointer to Arguments structure to free
 * */
void freeArguments(struct Arguments *args);

#endif
