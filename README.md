- [Install MSYS2](README.md#install-msys2)
- [Set up shortcuts to launch shells from PowerShell](README.md#set-up-shortcuts-to-launch-shells-from-powershell)
- [Get IMGUI](README.md#get-imgui)
- [Vim setup to navigate the IMGUI repo](README.md#vim-setup-to-navigate-the-imgui-repo)

# Install MSYS2

- download from https://www.msys2.org/
- take one (or both) of these precautions:
    - check the hash:
        - calculate the SHA256 on the downloaded file
        - PowerShell has the built-in command `Get-FileHash`

            ```
            Get-FileHash .\msys2-x86_64-20210604.exe
            2E9BD59980AA0AA9248E5F0AD0EF26B0AC10ADAE7C6D31509762069BB388E600
            ```

        - check the hash matches the SHA256 checksum on the website:

            ```
            $(Get-FileHash .\msys2-x86_64-20210604.exe).Hash -match "2e9bd59980aa0aa9248e5f0ad0ef26b0ac10adae7c6d31509762069bb388e600"
            ```

    - check the signature:
        - if you already have Cygwin installed on this computer
          use that to run `gpg`, if not download to a Linux
          computer and use `gpg` there, and if not that, the hash
          is good enough
        - it is not worth setting up `gpg` to do this from
          PowerShell
        - if you do have `gpg`:
            - use `gpg` to import the public key listed on the
              msys2.org website:

              ```
              gpg --recv-key 0xf7a49b0ec
              ```

            - download the GPG signature (should be a link just
              before the public key)
            - check that the downloaded file was signed with that
              key:

            ```
            gpg --keyid-format=long --with-fingerprint --verify msys2-x86_64-20210604.exe.sig msys2-x86_64-20210604.exe
            ```

            - look for "Good signature from"
            - don't worry about the warning: "WARNING: This key
              is not certified with a trusted signature!"

        - once MSYS2 is installed, you can run `gpg` from the
          `msys` shell to check signatures in the future

- run installer

Open the MSYS2 `msys` shell and run the package manager to update
system packages:

```
pacman -Syu
```

The `-y` flag downloads all the needed system packages. `-u`
upgrades.

Run the `-u` flag again to make sure everything is installed:

```
pacman -Su
```

# Set up shortcuts to launch shells from PowerShell

There are two shells:

- `msys`
    - use this for package management and POSIX stuff
- `mingw`
    - use this for Windows builds (like what we's gonna do with IMGUI)
    - stuff runs faster from this shell, e.g., Python

Instead of running from Desktop shortcuts, edit the PowerShell
Profile and create aliases to launch the two shells from
PowerShell:

```
Set-Alias -Name msys -Value "C:\msys64\msys2.exe"
Set-Alias -Name mingw -Value "C:\msys64\mingw64.exe"
```

This works but it may cause problems because it bypasses the
`msys2_shell.cmd` that triggers when clicking the Desktop
shortcuts, so it's not an identical way to run the shells because
some initialization steps will not run.

To avoid confusion in the future, the right way is to run the
shell and pass the desired shell type as a flag. This requires a
slightly fancier version of the `Set-Alias` command, passing a
function name to `-Value` instead of a string:

```
function RunMsys {
    C:\msys64\msys2_shell.cmd -msys
}
Set-Alias -Name msys -Value RunMsys

function RunMingw {
    C:\msys64\msys2_shell.cmd -mingw
}
Set-Alias -Name msys -Value RunMingw
```

# Install more packages

Open an `msys` shell.

Install these packages:

```
pacman -S make
pacman -S git
```

To check which packages you explicitly installed:

```
pacman -Qe
```

Things I also do at this point but which are not required to use
IMGUI:

Install Vim:

```
pacman -S vim
```

Clone my private `vim-dotvim` repo into folder `.vim`. Then, to
fully replicate my Vim setup, I clone all the Vim plug-ins like
this:

```
cd .vim
git submodule init
git submodule update
```

Install Python:

```
pacman -S python
```

I do this even if Python is already installed on Windows. I
cannot run the Windows Python from the `msys` or `mingw` shells.
This is not simply because the `$env:PATH` gets stripped. Even if
I inherit the full Windows `$PATH`, the Windows Python install
just doesn't run, don't know why.

Further, I like to source Python scripts from Vim. For some
reason, the command to do this is slightly different in Cygwin
bash and MinGW bash.

I have to change two things in my `.vimrc`:

1. `;so` (my shortcut for sourcing a script):
    - I use `!./%` under Cygwin
    - change it to `!%` under MinGW
2. sourcing a file works because of the Python shebang
    - Vim inserts the shebang for me
    - I use `usr/bin/env python3` under Cygwin
    - change it to `usr/bin/python` under MinGW

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
GLFW-Vulcan looks is more involved than a simple Makefile: it
looks like it requires MSVC (the Visual Studio compiler).*

Enter the directory of the GLFW-OpenGL example:

```
cd imgui/examples/example_glfw_opengl3/
```

The Makefile shows how to install the GLFW dependency on Windows
in MSYS. I copied those lines here:

```make
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
```

Open an `MSYS` shell and run the above package manager command to
install the dependency.

*Remember to do MSYS2 package management from the `MSYS` shell,
and do builds from the `MINGW` shell.*

Open a `MINGW` shell, enter this folder again, and run `make`:

```
make
```

The example builds and executable `example_glfw_opengl3.exe` is
created.

Open a Vim terminal (or another `MINGW` shell) and run it:

```
./example_glfw_opengl3.exe 
```

REMEMBER: build and run from MINGW *not* from MSYS2!!!

## Play with the example

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

Without going crazy making a tags file and a cscope database, Vim
`:find` and `:vimgrep` are good enough to navigate the IMGUI
repo.

- `:vimgrep` makes it easy to find the line number of the
- `:find` handles jumping to header files
  function signature

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

It's worth setting up tags file and cscope database to automate
all this a little better, but to get started, I think `:find`
(or, equivalently, shortcut `gf` with cursor on the `.h` file
name) is good enough.

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

Also note the path *must be a folder*. You cannot add a path to a
file.

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

This repository is an example of an IMGUI project. My source,
build output, and make-related files are all in the same flat
structure. But I put `imgui` stuff in its own folder.

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

