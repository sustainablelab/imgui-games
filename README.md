*This is for programming on Windows.*

- [Setup to program on Windows](README.md#setup-to-program-on-windows)
- [Get IMGUI](README.md#get-imgui)
- [Play with the example](README.md#play-with-the-example)
- [Vim setup to navigate the IMGUI repo](README.md#vim-setup-to-navigate-the-imgui-repo)

# Setup MSYS2 to program on Windows

Install [MSYS2](https://www.msys2.org/) and setup a C/C++
development environment. All the instructions are in [my MSYS2
notes](https://github.com/sustainablelab/msys).

My MSYS2 notes detail the MSYS2 installation and installing
essential packages (like `git`).

## What? And why?

From the MSYS2 website:

> MSYS2 is a collection of tools and libraries providing you with
> an easy-to-use environment for building, installing and running
> native Windows software.

MSYS2 provides three *subsystems*: `msys2`, `mingw32`, and
`mingw64`.

- I use subsystems `msys2` and `mingw64`.
- I make PowerShell aliases named `msys` and `mingw`
    - the aliases launch the **shell** associated with the
      subsystem
    - `msys` launches the shell for `msys2`
    - `mingw` launches the shell for `mingw64`

Use `msys` to manage packages and `mingw` for programming:

- `msys`
    - I only use this environment for maintenance
    - use this environment for package management and POSIX stuff
- `mingw`
    - most of the time I am working in this environment
    - use this environment for building Windows executables
        - e.g., building an application that uses IMGUI
    - stuff runs faster from this shell, e.g., Python

# Get IMGUI

Clone IMGUI:

```
git clone https://github.com/ocornut/imgui.git
```

## Install dependencies and build

The dependencies depend on which framework you choose to use.
IMGUI has examples for many frameworks. I choose
[GLFW](https://www.glfw.org/) using
[opengl3](https://www.opengl.org/).

*GLFW also supports Vulcan, but the IMGU build example for
GLFW-Vulcan is more involved than a simple Makefile: it looks
like it requires MSVC (the Visual Studio compiler).*

Enter the directory of the GLFW-OpenGL example:

```
cd imgui/examples/example_glfw_opengl3/
```

The Makefile shows how to install the GLFW dependency on Windows
in MSYS2. I copied those lines here:

```make
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
```

Open an `msys` shell and run the above package manager command to
install the dependency.

```bash-msys2
pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
```

*Remember to do MSYS2 package management from the `msys` shell,
and do builds from the `mingw` shell.*

Open a `mingw` shell, enter this folder again, and run `make`:

```bash-mingw64
make
```

The example builds and executable `example_glfw_opengl3.exe` is
created.

Open a Vim terminal (or another `MINGW` shell) and run it:

```bash-mingw64
./example_glfw_opengl3.exe 
```

REMEMBER: build and run from `mingw` *not* from `msys`!!!

# Play with the example

From the `imgui` repository, open:

    imgui/examples/example_glfw_opengl3/main.cpp

Locate the following line of code:

```c
ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.
```

Insert whatever you want to try out before that line. For
example:

```c
static float test_array_1[] = {1,2,3,4,99,5,6,7,8,9};
static float test_array_2[] = {8,7,8,9,85,11,14,11,8,29};
ImGui::Begin("Pile of leaves");

static float * test_array_active = test_array_1;

if (ImGui::BeginCombo(
        "label combo-box", // const char* label,
        "preview value", // const char* preview_value,
        0 // ImGuiComboFlags flags = 0
        )){

    if (ImGui::Selectable("selectable label data 1"))
    {
        test_array_active = test_array_1;
    }
    if (ImGui::Selectable("selectable label data 2"))
    {
        test_array_active = test_array_2;
    }
        ImGui::EndCombo();
}

ImGui::PlotLines(
        "plotty", // const char* label,
        test_array_active, //const float* values,
        sizeof(test_array_1)/sizeof(float), // int values_count,
        0, // int values_offset = 0,
        NULL, // const char* overlay_text = NULL,
        FLT_MAX, // float scale_min = FLT_MAX,
        FLT_MAX, // float scale_max = FLT_MAX,
        ImVec2(100, 100), // ImVec2 graph_size = ImVec2(0, 0),
        sizeof(float) // int stride = sizeof(float));
        );

ImGui::End();
```

How did I know about PlotLines and what it's function signature
is? I had help from someone who uses IMGUI to get me started
where to look. In addition, I set up Vim to navigate the IMGUI
repository.

# Vim setup to navigate the IMGUI repo

Out of the box, Vim has `:find` and `:vimgrep`. I show how to use
these. Then I show how to make a tags file and a cscope database
for even more powerful navigation.

## vimgrep and find

- `:vimgrep` finds occurrences of a search pattern, just like
  regular `grep` in bash, but makes it easy to jump to the
  matches:
    - after running the `:vimgrep`, open a quickfix window
      with `:copen`
    - press `Enter` on any match to jump to that location
    - jump to next/previous match with `Alt+Right` and `Alt+Left`
- `:find` finds files
    - I use `:find` to jump to header files
    - shortcut `gf` to jump to the file
    - shortcut `;w]`

## vimgrep

*Vimgrep example:*

In this example, I want to **find** all occurrences of `-D` in
this Makefile from the IMGUI examples:

    imgui/examples/example_glfw_opengl3/Makefile

*-D is a gcc flag that defines macros, so searching for `-D` in
the Makefile is a quick way to see what macros are defined.*

Open the Makefile in Vim. Then run this Vim command:

```vim
:vimgrep "\C -D" %
```

- I enclose the pattern in double-quotes
    - enclose the pattern in any non-ID character (do `:set
      isident?` to see the list of ID characters) as long as it
      does not appear in the pattern
- The `\C` makes the search case sensitive.
- The `%` means "the file in the active window"
    - test this with `:echo expand("%")`

## find

- `:find` then the header filename
- or put the cursor on the `.h` file name and use shortcut `gf`

To help `:find` find files in the `imgui` repo, add this to the
`.vimrc`:

```vim
let &path = &path . ',' . '/home/mike/imgui/**'
```

The trailing `**` tells Vim to recursive dive during `:find`.

To find standard library headers such as `stdio.h`:

```vim
let &path = &path . ',' . '/mingw64/x86_64-w64-mingw32/include/'
```

### Figure out library header paths

My trick to find the library header path is to use the gcc flag
`-M`. Here is a step-by-step example.

- create a dummy `.c` file:

```c
// find-headers.c
#include <stdio.h>
void main(void){}
```

- and create this Make recipe to print the library paths

```make
.PHONY: print-libs
print-libs: find-headers.c
	gcc $< -M > libs.txt
```

Run the `print-libs` recipe:

```bash
$ make print-libs
gcc find-headers.c -M > libs.txt
```

The paths are listed in file `libs.txt`:

```
$ cat libs.txt 
find-headers.o: find-headers.c \
 C:/msys64/mingw64/x86_64-w64-mingw32/include/stdio.h \
 ...
```

When adding `mingw64` paths to Vim variable `path`, omit the
`C:/msys64` prefix:

```vim
let &path = &path . ',' . '/mingw64/x86_64-w64-mingw32/include/'
```

Also note:

- the path *must be a folder*
    - you cannot add a path to a file
- the trailing `/` is optional
- the leading `/` is **not** optional
    - for Vim `find` to find files, the way to specify a path
      starting with `C:/msys64/mingw64` is to start the path as
      `/mingw64/`
    - note the leading `/`!
    - `mingw64/` will not work

## ctags

To setup a ctags file, first open an `msys` shell, then install
`ctags`:

```
pacman -S ctags
```

Enter the `imgui` repo and run `ctags`:

```
ctags --c-kinds=+l --exclude=Makefile -R .
```

This creates a `tags` file. The `tags` is in the top-level folder
of the `imgui` repo clone. Similar to the Vim `path` variable
used by `:find`, there is a `tags` variable used by `:tag`
commands. This `tags` variable tells Vim where to look for `tags`
files. Add this to the `.vimrc`:

```vim
let &tags = &tags . ',' . '/home/mike/imgui/tags'
```

Now the Vim tag-hopping shortcuts will work. For example, with
the cursor on a tag name, like a header file or a function,
`Ctrl+]` jumps to the file or to the location where the function
is defined.

And since IMGUI is C/C++, and Vim omni-complete uses the `tags`
files for omni-completion of C/C++ file types, `Ctrl+X Ctrl+O` in
insert mode will pop-up an auto-complete menu and open a preview
window with the signature of the highlighted function.

# New IMGUI project

This Git repository is an example of an IMGUI project. So first
off, I start a project by creating a Git repository. It's easier
to do this first because I'm going to add IMGUI as a Git
submodule.

## Flat file structure

My source, build output, and make-related files are all in the
same flat structure. They all just live together at the top of my
project folder.

## Clone IMGUI as a submodule

The IMGUI dependencies are all in the IMGUI repository. The best
approach is to clone this repository into each project as a Git
submodule. 

```bash
cd imgui-games
git submodule add https://github.com/ocornut/imgui.git
```

Now I have folder `imgui` in my project folder. This folder is
the clone of the IMGUI repository.

Git creates a `.gitmodules` file at the top-level:

```.gitmodules
[submodule "imgui"]
        path = imgui
        url = https://github.com/ocornut/imgui.git
```

And Git creates a  `modules` folder in my project's `.git`
folder. My project is the *superproject*. The superproject has a
`.git` folder and the submodule has a `.git` file.

- The IMGUI submodule has its *Git directory* (the magic files
  that track changes) in the `./.git/modules` folder.
    - file `./imgui/.git` lists the path to this *Git directory*:

        ```
        gitdir: ../.git/modules/imgui
        ```

- The IMGUI submodule has its *working directory* (the files I am
  actually using) in `imgui-games/imgui`.
    - Since `imgui-games` is the *superproject* (this is what the
      Git Manual calls the top-level project), the path to the
      working directory is simply `imgui`.
    - Recall that file `.gitmodules` says `path = imgui`. So
      `.gitmodules` shows the path to the *working directory*.

Lastly, adding a submodule is a change to my project's
repository. I commit this like any other change.

But when IMGUI changes and I pull the latest version, my
superproject still points at the commit I initially cloned.

```
$ git status
On branch master

Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
  (commit or discard the untracked or modified content in submodules)
        modified:   imgui (modified content)

no changes added to commit (use "git add" and/or "git commit -a")
```

How do I tell the superproject that it's using the submodules'
latest commit?

## Build setup

`IMGUI` is intended to be built from source. That means I
generate IMGUI object files when I build my project.

Should those object files go in the `imgui` submodule? No.

Following how the IMGUI examples do it, let the object files that
get generated by my build go in my project's build folder. Since
I don't bother making a separate build folder, that means all the
object files dump directly into the same folder with my source
code and my final .exe.

## make tags

I put two tags recipes in my Makefile.

This is my recipe for tags in my source code:

```make
.PHONY: tags
tags: main.cpp
	ctags --c-kinds=+l --exclude=Makefile -R .
```

Now I generate the tags file with `make tags`.

I also want tags for definitions in the library dependencies.
Here is my recipe:

```make
.PHONY: lib-tags
lib-tags: main.c
	gcc $(CFLAGS) $< -M > headers-windows.txt
	python.exe parse-lib-tags.py
	rm -f headers-windows.txt
	ctags -f lib-tags --c-kinds=+p -L headers-posix.txt
	rm -f headers-posix.txt
```

This uses the same gcc `-M` flag trick to get the list of library
paths:

- A Python script does some simple reformatting so that `ctags`
  understands the list of paths (I have to remove non-trailing
  whitespace and use newline as the only delimiter).
- The `-L` flag tells ctags to read from the `.txt` file the list
  of file names to generate tags for.

