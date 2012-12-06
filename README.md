# Direct Graphics API for Open Source Platforms

I made this library to make a portable layer over what raspberry pi provides. EGL and OpenWF still leave implementation-specific gaps into the specification that will backfire if real portability over ARM platforms is attempted. There's also some legacy in EGL that I don't particularly like or understand.

Current version of this library is still bit unstable. The errors are uncharted and might not be detected. Whenever bugs occur we'll patch them and add error codes.

## Features

 * Easy to use
 * Has semantic versioning, new official functions will bump the minor version, changes to existing functions bump the major version.
 * Platform specific functions are postfixed. These are considered 'unstable' way of getting things done.

## How To Install

 * Run the compile script inside this directory, after compiling you should have libdg.so and bin/ in the directory.
 * You can try the examples in bin/ -directory, instructions later.
 * copy libdg.so into /usr/lib/ and contents of include/ into /usr/include/

## How to Try

    ./compile
    LD_LIBRARY_PATH="." bin/print_resolution_brcm
    LD_LIBRARY_PATH="." bin/fullscreen_brcm

## Documentation

Read the `include/DG/dg.h` and tests through. This API doesn't have other documentation yet.
