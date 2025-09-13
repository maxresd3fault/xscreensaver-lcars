# XScreenSaver LCARS

**Quick Overview**:
This is a custom screensaver wrapper for XScreenSaver (also compatible with MATE Screensaver) running the Flash file from the [System 47 Project](https://www.mewho.com/system47/). All credit goes to System47 for making the Flash animations and graphics, this program is simply a wrapper to run said Flash file. Unfortunately System47 offers no Linux binary for their screensaver. Some people have used Wine to run System47 on their Linux boxes, however, this is somewhat unnecessary because System47 is just a Flash program. Extracting the `.swf` file from the program, we can use something like Ruffle to run it natively on Linux.

Please also keep in mind XScreenSaver was really never designed to be used in this way. This wrapper is quite hacky in the way it remaps Ruffle's window to the XScreenSaver window and at this point still isn't fully stable.

The wrapper also logs to `~/.lcars-log`

**Requirements**:
* [XScreenSaver](https://www.jwz.org/xscreensaver/)
* [Ruffle](https://ruffle.rs/) (Flash emulator)

**Installation**
Simply download the latest `lcars.tar.gz` from releases, move it to your root directory, and then run `sudo tar -xzvf lcars.tar.gz`

**Compiling:** 
To compile the wrapper binary you can run `gcc -o lcars-bin lcars-bin.c -lX11`

**Bugs/Limitations**:
* No audio (limitation of XScreenSaver)
* Sometimes the lcars-bin process doesn't end - currently working on a fix
