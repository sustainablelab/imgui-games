# Folders

## imgui-old

`imgui-old/` was my first attempt at using IMGUI:

- IMGUI obviously has files it needs
    - these files are in the IMGUI repository
    - but this repository contains files for EVERY flavor of
      build, plus examples

I figured the right way to go was to figure out the parts I need
and just copy those.

- I copied only the files that were absolutely necessary into
  this folder, now named `imgui-old`
- I set up the build to use these files
- I set up the build to put IMGUI object files in this folder in
  a `build/` subfolder

I quickly realized this is the WRONG way.

The right way is to just clone the IMGUI repo into the project.
The repo is updated often. Copying the files out again is a lot
of work.

## submodules

Put my Git submodules here.
