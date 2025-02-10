#ifndef PROCESS_H
#define PROCESS_H

#include "args.h"
#include "errors.h"
#include <stdio.h>

enum FileType { FILE_TYPE_UNKNOWN, FILE_TYPE_DIRECTORY, FILE_TYPE_REGULAR };

/* *
 * Index state stores the path to an index file as well as a file pointer to
 * the open index file. It is used with FileIterator to keep track of and read
 * index files while traversing a project.
 * */
struct IndexState {
    char *curIndexFileDir;
    FILE *curIndexFile;
};

/* *
 * FileIterator is a stack that keeps track of the programs position in a
 * project as well as providing limits for the project depth and a status to
 * indicate if an error has occured in the process of traversing the project.
 *
 * NOTE: stackMax will eventually be user-configurable but for now defaults to
 * 5 levels of nesting, which is what I've determined to be sufficient based on
 * my prejudices against complexity.
 * */
struct FileIterator {
    struct IndexState *stack; // pointer == array
    size_t stackSize;
    size_t stackMax;
    enum FileIteratorStatus status;
};

/* *
 * ProcessContext keeps track of the project files and their types as they are
 * being processed. It also contains the output file pointer (if collate mode
 * is being used), the output path as well as the status of the context to halt
 * if there is an error. The name of the output file or directory can be set by
 * the user, otherwise it will default to _draft_.
 * */
struct ProcessContext {
    char *currentFilePath;
    char *outPath;
    FILE *outFile;
    enum FileType currentFileType;
    enum ProcessContextStatus status;
};

/* *
 * ProjectState contians all the context needed for the program to process a 
 * project. It contains the above FileIterator and ProcessContext to keep track
 * of the program's place in both index and project files. It also contains a
 * handler function that is determined based on the processing mode chosen by
 * the user as well as a status to identify if any errors occur during 
 * processing. 
 * */
struct ProjectState {
    struct ProcessContext context;
    struct FileIterator iter;
    enum FileHandlerStatus (*handlerFunction)(struct ProcessContext *context);
    enum ProjectStateStatus status;
};

/* *
 * Initializes state based on arguments passed by the user. Iterates through 
 * index files, processing project files in the order they appear in
 * the index files.
 *
 * @param   args  Command line arguments included when the program is executed.
 *
 * @return  int
 *          0     if process completes without and errors.
 *         -1     if any error occurs during the process including:
 *                    - state intitialization
 *                    - index file traversal
 *                    - project file traversal/processing
 *                    - output writing
 * */
int processProject(struct Arguments *args);

#endif
