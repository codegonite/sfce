# Single File C Editor (SFCE)

This is a text editor written using the C programming language based on the [piece tree](https://code.visualstudio.com/blogs/2018/03/23/text-buffer-reimplementation) data structure from visual studio code. Building this program was a challenge to create a command line text editor all within a single file of C code.

## Overview

This text editor currently only allows for text editing small files as it doesn't support scrolling or a text search feature. Most of the code uses ansi escape sequences for terminal interaction but, there is still a dependency on the WIN32 API; Meaning this code should only support windows.

## Getting Started

Here are a set of instructions for getting this project setup locally on your computer.

**Prerequisites**

The GCC C compiler and the make command are the only requirements needed to build this project

**Clone the repository:**

```bash
git clone https://github.com/codegonite/sfce.git
cd sfce
```

**Then build using the make command:**

```bash
make
```

**Or using the GCC compiler**

```bash
gcc sfce.c -Os -Werror -Wfatal-errors -Wall -o sfce
```

## Usage

```bash
sfce path/to/file
```

## References

- [Piece Tree Data Structure](https://code.visualstudio.com/blogs/2018/03/23/text-buffer-reimplementation)
- [Unicode Text](https://www.unicode.org)
