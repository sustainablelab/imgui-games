SNAD is an open-source game. Still calling the executable `bob`,
so still early days.

## Builds on Linux and Windows

Build the Windows version with the `mingw64` subsystem of MSYS
(no VisualStudio required).

The Windows build is in the `windist` folder and does not require
installing MSYS to run it (I think... I still has to test this...).

## IMGUI for debug

Build the version without the debug GUI (like what's in the
`windist` folder):

```bash
make
```

Or build the DEBUG version:

```bash
git submodule update --init # Do this once to download IMGUI
make DEBUG=yes
```

## To build on Windows

*I has to clean this README up now that bob's a snad.*

Install these packages to build on Windows. Open an MSYS terminal
and do this:

```bash
pacman -S mingw64/mingw-w64-x86_64-glew
pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
pacman -S mingw-w64-x86_64-openal
pacman -S mingw-w64-x86_64-freealut
pacman -S mingw-w64-x86_64-freetype
```

And [here's my notes](my-notes.md).
