# Colette

A command-line tool for organizing and collating text files in writing projects.

## Overview

Colette helps writers manage multi-file writing projects by providing tools to organize and combine text files based on index files that define the desired order. It's particularly useful for writers who break their work into smaller, manageable files but need to easily combine them for review or export.

## Status

⚠️ **Early Development** - Core features are under active development

## Features (Planned)

- Recursive file collation based on index files
- Automatic project initialization
- Project structure validation
- Support for infinitely nested scene structures
- Markdown support (other markup languages planned)

## Usage

```bash
# Initialize a writing project and generate index files
colette --init path/to/project

# Collate files in a directory according to its index
colette path/to/project

# Validate project structure
colette --check path/to/project
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

## Requirements

- POSIX-compliant system (Linux, MacOS)
- C compiler (gcc recommended)
- make

## License

[License details to be determined]

