![Screenshot](https://i.imgur.com/tjzXT6G.png)

# GuitarD

It's a basic multi effects processor which follows a node based approach.
It's fairly unstable and experimental but contains most of the important features.
This whole thing started as my bachelors thesis, but I plan on developing it further to a point where it's usable.

[Super rough demo of a few tones/effects that come with it](https://vocaroo.com/3kVGNkkApqR)

## Builds
[See here](https://github.com/TobiasKozel/GuitarD/releases)

## I'm not responsible for any crashes or hearing loss. (Seriously, be careful)

## The preset and cabinet library go online to fetch thier stuff.
So if a preset sounds different it might be because it couldn't fetch the IRs from the server.
You can manually copy the IRs in the GuiutarD/ir_cache in your systems homefolder.

## How to use
In progress. Have a look at `./manual/guitard manual.pdf`

## Compilation
### The Plugin
To build a plugin you'll need [this fork of iPlug2](https://github.com/TobiasKozel/iPlug2) and switch to the `guitard` branch there.

To get iPlug2 up and running, make sure all its dependencies are downloaded. Run both of these shell scripts (use git bash on windows):

`iPlug2/Dependencies/download-prebuilt-libs.sh`

`iPlug2/Dependencies/IPlug/download-iplug-libs.sh`

After that, simply clone this repository into the Examples folder of iPlug2 and open the IDE project for your platform.

You'll still have to generate the C++ code from FAUST as described below.

The VST/AU plugin works on Windows and Mac OS X. For 32 Bit support another renderer than SKIA has to be used. Head over to the iPlug2 Wiki for more info about the graphic backends.

### Only the DSP

Make sure the [FAUST compiler](https://github.com/grame-cncm/faust/releases) is installed and in your PATH environment variable (this should be the default case). Run `python ./scripts/compile_faust.py` to compile all the DSP code.

The DSP code can be compiled without iPlug and the GUI so it can be included in other projects easily. Just include `./src/headless/headless.h` and you should be good to go. Everything is header only to make the code as portable as possible. The headless version was testet on Windows (MSVC, gcc), Mac OS X (clang) and Linux (gcc, clang).

A small demo using the systems default audio devices as in and output can be found here:

https://github.com/TobiasKozel/GuitarD/blob/master/src/headless/device.cpp

The readme besides it contains some info about compiling it.

There are currently some things that might need fixing, in order for the code to be as robust as a header only library should be, since I'm no C++ expert.
Mainly [#6](https://github.com/TobiasKozel/GuitarD/issues/6) and [#7](https://github.com/TobiasKozel/GuitarD/issues/7)
