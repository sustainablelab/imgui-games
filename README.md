*This is for programming on Windows.*

- [Setup to program on Windows](README.md#setup-to-program-on-windows)
- [Get IMGUI](README.md#get-imgui)
- [Play with the example](README.md#play-with-the-example)
- [Vim setup to navigate the IMGUI repo](README.md#vim-setup-to-navigate-the-imgui-repo)
- [Git submodules](README.md#git-submodules)
- [Use IMGUI in a project](README.md#use-imgui-in-a-project)

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

## What? And why?

Without IMGUI, I can already put pixels and primitive shapes on
the screen with SDL2, e.g., [in this
repo](https://github.com/sustainablelab/falling-something) I use
SDL2 directly to noodle with the concept behind the [Falling
Everything Engine](https://nollagames.com/fallingeverything/).

IMGUI is a level above SDL2. In fact, IMGUI uses libraries like
SDL2 as the **platform backend**. SDL2 is one of the backend
options for IMGUI. See imgui/docs/BACKENDS.md.

IMGUI provides GUI stuff: drop-downs, sliders, etc. Run the
example (follow the instructions below) to see all the things
IMGUI can do.

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

## summary of vimgrep and find

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
    - shortcut `;wf` to open the file in a new window

`:find` only looks on the Vim `path`. I use `gcc -M` in a
Makefile recipe to figure out the header paths used in this
project. Then I add those paths to the Vim `path` by editing the
`.vimrc`.

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

- the path *must be a folder-path*
    - you cannot add a file-path to Vim path
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

Like anything else, it's nice to automate tag generation/updating
with the Makefile.

## make tags

I put two tags recipes in my Makefile.

This is my recipe for tags in my source code:

```make
.PHONY: tags
tags: main.cpp
	ctags --c-kinds=+l --exclude=Makefile -R .
```

Now I generate/update the tags file with `make tags`.

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

Now I generate/update the lib-tags file with `make lib-tags`.
Note this is a separate tags file. I tell Vim to search both
files by setting `tags` in my `.vimrc` like this:

```vim
set tags=./tags,tags,lib-tags
```

The `lib-tags` recipe uses the `gcc -M` trick to get the list of
library paths:

- A Python script does some simple reformatting so that `ctags`
  understands the list of paths (I have to remove non-trailing
  whitespace and use newline as the only delimiter).
- The `-L` flag tells ctags to read from the `.txt` file the list
  of file names to generate tags for.

The Python script that formats the header paths is very short:

```python
import pathlib
with open("headers-windows.txt") as fin:
    with open("headers-posix.txt", mode="w") as fout:
        for line in fin:
            for s in line.split():
                if '.h' in s:
                    fout.write(str(pathlib.PurePosixPath(s)))
                    fout.write("\n")
```

# Git submodules

I don't use Git submodules often, so the entire next section is
about Git submodules. This is not specific to IMGUI, but that's
what I use in all the examples.

## Add IMGUI as a submodule

The IMGUI dependencies are all in the IMGUI repository. Clone
this repository into each project as a Git submodule. For
example:

```bash
$ cd imgui-games
$ git submodule add https://github.com/ocornut/imgui.git
Cloning into '/home/mike/gitrepos/imgui-games/imgui'...
remote: Enumerating objects: 42602, done.
remote: Counting objects: 100% (899/899), done.
remote: Compressing objects: 100% (324/324), done.
remote: Total 42602 (delta 690), reused 744 (delta 573), pack-reused 41703
Receiving objects: 100% (42602/42602), 75.32 MiB | 13.69 MiB/s, done.
Resolving deltas: 100% (32232/32232), done.
```

Now I have folder `imgui` in my project folder. This folder is
the clone of the IMGUI repository.

**This is a change to the repository. Remember to commit it!**

I commit adding a submodule like I would for any other change:

```bash-git
git add .
git commit -m 'Add IMGUI as Git submodule'
```

Or if I have other changes and just want to stage the addition of
the submodule in this commit, I need to stage two items:

- the `.gitmodules` file
- the `imgui` folder

So I do this:

```bash-git
git add .gitmodules imgui
git commit -m 'Add IMGUI as Git submodule'
```

TODO: How do I tell the superproject to use the submodule's
latest commit?

When IMGUI changes and I pull the latest version, my
superproject still points at the commit I initially cloned.

```bash-git
$ git status
On branch master

Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
  (commit or discard the untracked or modified content in submodules)
        modified:   imgui (modified content)

no changes added to commit (use "git add" and/or "git commit -a")
```

So does `git submodule update` do this?

If not, then I think the way to do this is:

```bash-git
git submodule update --remote --merge
```

### What adding as a submodule means in practice

#### .gitmodules file

Git creates a `.gitmodules` file at the top-level:

```.gitmodules
[submodule "imgui"]
        path = imgui
        url = https://github.com/ocornut/imgui.git
```

#### modules folder

And Git creates a  `modules` folder in my project's `.git`
folder. My project is the *superproject*. The superproject has a
`.git` folder and the submodule has a `.git` file.

- The IMGUI submodule has its *Git directory* (the magic files
  that track changes) in the `./.git/modules` folder.
    - file `./imgui/.git` lists the path to this *Git directory*:

        ```
        gitdir: ../.git/modules/imgui
        ```

#### working directory

- The IMGUI submodule has its *working directory* (the files I am
  actually using) in `imgui-games/imgui`.
    - Since `imgui-games` is the *superproject* (this is what the
      Git Manual calls the top-level project), the path to the
      working directory is simply `imgui`
    - Recall that file `.gitmodules` says `path = imgui`. So
      `.gitmodules` shows the path to the *working directory*.

The useful takeaway about the working directory is for recipes in
the Makefile. The recipes have to be able to find IMGUI. Paths to
IMGUI dependencies start with `imgui/`.

This assumes my Makefile is at the project top-level, which it
should be because I want someone to simply clone my project and
be able to run `make` from the top-level folder.

### Temporarily stop using a submodule

```bash
git submodule deinit imgui
```

The `deinit` removes the files from the working tree (deletes
them from disk) and tells Git to stop looking at this submodule
when doing submodule updates.

It's as if it's gone, but it's not really gone because you can
just `git submodule init` to bring it back.

The `deinit` does not remove working tree files that contain
local modifications. To force the removal (throw away local
changes) use `--force` or `-f`.

### Really delete a submodule

Use `git rm`:

```
$ git rm imgui
$ git commit -m 'Remove submodule imgui'
```

If there are uncommitted changes, add these flags:

```
$ git rm -f --cached imgui
$ git commit -m 'Remove submodule imgui'
```

You still need to manually delete the `imgui` folder to
completely remove any trace of the submodule.

```bash
rm -rf imgui
```

This is because Git keeps the folder around to store the
submodule's `.git` file to make it possible to checkout past
commits without fetching from the submodule's remote.


### Submodule not modified but shows as untracked content

Here's something annoying that happens. Using Vim creates swap
files and undo files. As I poke around files in the submodule, I
inadvertently create these hidden files. Git sees this as
modifications to the working tree in the submodule and tells me I
have untracked content:

```bash-git
$ git status
On branch master
Your branch is up to date with 'origin/master'.

Changes not staged for commit:
  (use "git add/rm <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
  (commit or discard the untracked or modified content in submodules)
        modified:   imgui (untracked content)
```

To see the problem explicitly, run `git status` from the `imgui`
folder:

```bash-git
$ cd imgui
$ git status
On branch master
Your branch is up to date with 'origin/master'.

Untracked files:
  (use "git add <file>..." to include in what will be committed)
        examples/example_glfw_opengl3/.Makefile.un~

nothing added to commit but untracked files present (use "git add" to track)
```

Usually I ignore this Vim undo file with my `.gitignore`, but the
`.gitignore` only affects my git module, not the git submodules.

I cannot edit the `.gitignore` in the `imgui` submodule -- Git
will ignore the undo file, but then it will show that I've
modified the submodule because I changed the `.gitignore`.

Why didn't `ocornut` ignore Vim files for me? It's considered bad
practice to put non-project-specific ignore files in the
`.gitignore`.

The "good practice" is to create a global `.gitignore` file
somewhere, usually in the HOME directory:

```
vim ~/.gitignore
```

And list the generic items-to-ignore in that global `.gitignore`
file:

```.gitignore
*~
*.sw?
```

Then tell Git to use the global `.gitignore`:

```bash-git
git config --global core.excludesfile ~/.gitignore
```

List the global config to see Git is using the global
`.gitignore` file:

```bash-git
$ git config --global -l
core.autocrlf=input
core.editor=vim
core.excludesfile=/home/mike/.gitignore
user.name=sustainablelab
user.email=sustainablelab@gmail.com
```

And now Git does not see any modifications to the submodule:

```bash-git
$ git status
On branch master
Your branch is up to date with 'origin/master'.
```

## Cloning a project that has submodules

Before getting into making a project, there is one more point
about Git submodules. What happens when someone wants to use my
project (or I want to run it another machine)?

First they clone it like usual:

```bash-git
git clone https://github.com/sustainablelab/imgui-games.git
```

But the IMGUI submodule (and any other submodules) show up
empty, so the project cannot build.

Clone the submodules like this:

```bash-git
git submodule update --init
```

This one-liner works for all my submodules, not just IMGUI.

The one-liner is equivalent to these two steps:

```bash-git
git submodule init
git submodule update
```

The `init` "registers" the submodules by editing my `.git/config`
while the `update` clones the registered submodules. *I do this in
two steps when I want to customize the clone URLs in
`.git/config` in my local setup. I do that customization after
the `init`, but before the `update`.*


# Use IMGUI in a project

At this point:

- I have a working MSYS2 environment that builds native Windows
  executables.
- I have a Vim environment that makes it easy to navigate project
  code and use auto-completion.
- I know the basics of using IMGUI just from playing with the
  examples it comes with, or at least I've successfully built an
  example and played with it.
- I also know how to work with Git submodules.

*Now I am ready to create a new project that uses IMGUI.*

This Git repository is an example of a project that uses IMGUI.

- start a project by creating a Git repository
- add IMGUI as a Git submodule

Each project has its own IMGUI git submodule. So lots of copies
of IMGUI on your computer? Yes, that's OK.

Really, **this is the right way to do this.**

- The IMGUI submodule does not take up space at the Git remote.
- The IMGUI submodule does not need to take up space on the local
  clone either -- you only need it if you are building, so if
  space is really an issue you can just `deinit` the submodule
- on the GitHub website, clicking on the folder jumps to the
  repository on the `ocornut` page (the IMGUI author)

## Use a flat file structure

My source, build output, and make-related files are all in the
same flat structure. They all just live together at the top of my
project folder.

## Build setup

`IMGUI` is intended to be built from source. That means I
generate IMGUI object files when I build my project.

Should those object files go in the `imgui` submodule? No.

Following how the IMGUI examples do it, let the object files that
get generated by my build go in my project's build folder. Since
I don't bother making a separate build folder, that means all the
object files dump directly into the same folder with my source
code and my final .exe.

## Makefile

The Makefile is only these 19 lines once all the parts for other
OS are removed. It divides into three parts.

The first part defines the names of the object files based on the
source code name `main.cpp` and the executable name `bob.exe`.

```make
EXE = bob.exe
default-target: $(EXE)
IMGUI_DIR = imgui
SOURCES = main.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
```

Next, the compiler flags and library dependencies are defined.

```make
CXXFLAGS = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -g -Wall -Wformat
CXXFLAGS += `pkg-config --cflags glfw3`
LIBS = -lglfw3 -lgdi32 -lopengl32 -limm32
```

Finally, the actual make recipes use pattern matching (the
`%.o:%.cpp`) and automatic variables (the `$@` and `$<`) to
result in terse recipes.

```make
%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)
```

To see what value a variable takes on, create a recipe like this:

```make
.PHONY: list-objs
list-objs:
	@echo $(OBJS)
```

Then running `make list-objs` prints out the list of object
files.

To see what the recipes do, run `make -n`:

```bash-make
$ make -n
g++ -Iimgui -Iimgui/backends -g -Wall -Wformat `pkg-config --cflags glfw3` -c -o main.o main.cpp
g++ -Iimgui -Iimgui/backends -g -Wall -Wformat `pkg-config --cflags glfw3` -c -o imgui.o imgui/imgui.cpp
g++ -Iimgui -Iimgui/backends -g -Wall -Wformat `pkg-config --cflags glfw3` -c -o imgui_demo.o imgui/imgui_demo.cpp
g++ -Iimgui -Iimgui/backends -g -Wall -Wformat `pkg-config --cflags glfw3` -c -o imgui_draw.o imgui/imgui_draw.cpp
g++ -Iimgui -Iimgui/backends -g -Wall -Wformat `pkg-config --cflags glfw3` -c -o imgui_tables.o imgui/imgui_tables.cpp
g++ -Iimgui -Iimgui/backends -g -Wall -Wformat `pkg-config --cflags glfw3` -c -o imgui_widgets.o imgui/imgui_widgets.cpp
g++ -Iimgui -Iimgui/backends -g -Wall -Wformat `pkg-config --cflags glfw3` -c -o imgui_impl_glfw.o imgui/backends/imgui_impl_glfw.cpp
g++ -Iimgui -Iimgui/backends -g -Wall -Wformat `pkg-config --cflags glfw3` -c -o imgui_impl_opengl3.o imgui/backends/imgui_impl_opengl3.cpp
g++ -o bob.exe main.o imgui.o imgui_demo.o imgui_draw.o imgui_tables.o imgui_widgets.o imgui_impl_glfw.o imgui_impl_opengl3.o -Iimgui -Iimgui/backends -g -Wall -Wformat `pkg-config --cflags glfw3` -lglfw3 -lgdi32 -lopengl32 -limm32
```

9 recipes run in total. The first 8 recipes are generating the
object files from the `.cpp` source code. The final recipe links
the object files to generate the executable. Pretty standard
stuff.

# Write main.cpp

Finally here. What's the boilerplate `main.cpp` file to get started?

Start with the necessary headers. These are the IMGUI headers
found in the Git submodule:

```c
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
```

And these are headers provided by MSYS2 packages:

```c
#include <stdio.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
```

With the backend I'm using, the Makefile does not define the
macro `IMGUI_IMPL_OPENGL_ES2`, so I drop `#if defined` and do not
`#include <GLES2/gl2.h>`.

The code is all calls to either the IMGUI library or to GLFW.
IMGUI stuff starts with `ImGui` or `IMGUI` and GLFW starts with
`glfw`.

Functions starting with `glfw` are defined by GLFW. The
prototypes for these functions are in the `GLFW/glfw3.h` header.
On my MSYS2 installation, this is in
`C:/msys64/mingw64/include/GLFW/glfw3.h`.

For example, the `main` loop terminates when the window's **close
flag** is set:

```c
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ...
    }
```

Here is the Doxygen in `glfw3.h` for `glfwWindowShouldClose()`:

```c
/*! @brief Checks the close flag of the specified window.
 *
 *  This function returns the value of the close flag of the specified window.
 *
 *  @param[in] window The window to query.
 *  @return The value of the close flag.
 *
 *  @errors Possible errors include @ref GLFW_NOT_INITIALIZED.
 *
 *  @thread_safety This function may be called from any thread.  Access is not
 *  synchronized.
 *
 *  @sa @ref window_close
 *
 *  @since Added in version 3.0.
 *
 *  @ingroup window
 */
GLFWAPI int glfwWindowShouldClose(GLFWwindow* window);
```

The application divides into three parts: setup, loop, cleanup.

Setup is creating the Window where the application lives. This
window can be fullscreen or windowed.

Then an infinite loop begins which continues until it sees the
window has been flagged for closing.

Finally, there is cleanup to destroy the window, free memory, and
exit.

# Build

I build and run from Vim.

Build the executable:

```vim
:make
```

Run the executable

```vim
:!./bob.exe
```

Build tags and lib-tags:

```vim
:make tags
:make lib-tags
```

The advantage of invoking `make` from the Vim command line is
that `:copen` opens the quickfix window where it is easy to jump
to the line of code that caused an error or warning.
