# Simple Filesystem

Simulated UNIX File System Project for Operating Systems course.

## How to Build and Use

To build the code, install either CLion (from Jetbrains) or cmake (with your package manager).

The project also requires the essential build tools (make, C compiler, linker, assembler, etc.) so make sure
you also have these installed (e.g. use `sudo apt-get install build-essential` on Ubuntu).

To build with CLion, open the project, select the `sfstest` target, and click the green "Build and Run" button.
If you don't get any output in the console, this is because of a bug in CLion.
Run the generated `sfstest` program in your terminal and it will work correctly.

To build with cmake, use the following commands after `cd`'ing to the project root.

    mkdir build
    cd build
    cmake ..
    make
    
The test program (`sfstest`) is now in the `build/tests` directory and can be executed.

There is also an included unit test suite, which generates an executable called `tests`.
All tests in the suite should pass.

## Table of Contents

- `doc/`    - Project documentation.
- `src/`    - All source files needed to compile the filesystem library.
- `tests/`  - All tests.
- `sfs.h`   - Header that defines the public interface of the filesystem.

## Authors

Rob, Aaron, Matt, and Pat wrote all the code in this project unless otherwise noted.

Responsibility for the different functions was divided as follows:

- Rob   - `sfs_open`/`sfs_close`
- Aaron - `sfs_read`/`sfs_write`
- Matt  - `sfs_create`/`sfs_delete`
- Pat   - everything else