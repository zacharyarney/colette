# Colette

A command-line tool for organizing and collating text files in writing projects.


## Contents

- [Overview](#overview)
- [Status](#status)
- [Usage](usage)
- [Project Structure](#project-structure)
- [Installation](#installation)
- [Features](#features-planned)
- [Requirements](#requirements)
- [License](#license)


## Overview

Colette helps writers manage multi-file writing projects by providing means to organize and combine text files based on index files that define the desired order. It's particularly useful for writers who break their work into smaller, manageable files but need to easily combine them for review or export.

Default behavior, with no flags, will assume a project is initialized and will traverse the specified directory and its subdirectories, collating each project file into a single "draft" using the index files to determine the order project files are processed.

Initialize a project with the `--init` flag. This will traverse the project and generate index files in every directory. The index files will mirror the system file structure, with each project file on its own line. The order of the text in the collated output is determined by the order specified in these index files. Rearrange the file names in the index files to structure the project. This way, project structure is decoupled from the computer's file system. Project file and directory names can be purely descriptive of scenes and chapters without having to worry about naming them sequentially. No more renaming every file when you insert a new scene!

Use the `--check` flag to avoid generating or overwriting the output file. Used in tandem with `--init`, it will generate index files without needlessly creating an output. Used on its own, it will verify the project structure is valid for colette to run.

The `--title` flag allows you to specify the name of the output file. This is helpful for generating multiple drafts.


## Status

⚠️ **Early Development** - Core features are under active development

Colette currently assumes projects consist of markdown and txt files. Mixing file types will affect the order that project files are added to index files. It is recommended to stick with one or the other. The output file will always be `.md`.


## Usage

```bash
# Recursively collate files in a directory in order specified by index files
colette path/to/project

# Initialize a writing project, generate index files and output file
colette --init path/to/project

# Validate project structure without generating an output file
colette --check path/to/project

# Name the output file
colette --title custom_title path/to/project

# Initialize project without generating an output file
colette -ic path/to/project
```


## Project Structure

Colette expects writing projects to follow this structure:

```
project/
├── .index              # Root index file listing chapters
├── chapter-1/          # Chapter directory
│   ├── .index         # Chapter index file listing scenes
│   ├── scene-1.md     # Scene file
│   └── scene-2.md
└── chapter-2/
    ├── .index
    └── complex-scene/  # Scenes can be directories
        ├── .index     # Scene index file listing subscenes
        ├── part-1.md
        └── part-2.md
```

Files or directories prefixed with `_` are ignored during collation.


## Installation

Currently requires manual compilation. Installation instructions coming soon.

Clone this repository, `cd` into the root directory and run `make release`. Copy binary into `~/bin/` or preferred directory in your PATH.


## Features (Planned)

- [x] Recursive file collation based on index files
- [x] Automatic project initialization
- [x] Project structure validation
- [ ] Symlink list alternative to single collated file
- [ ] Support for infinitely nested scene structures
- [x] Markdown support (other markup languages planned)


## Requirements

- POSIX-compliant system (Linux, MacOS)
- C compiler (gcc recommended)
- make


## License

This project is licensed under the MIT License - see the [LICENSE](https://github.com/zacharyarney/colette/blob/feat/init/LICENSE) file for details.

