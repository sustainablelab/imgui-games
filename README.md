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

Start with this example:

```
cd imgui/examples/example_glfw_opengl3/
```

The Makefile shows how to install the OpenGL dependency on
Windows in MSYS:

```make
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
```

That's it.

Run `make`:

```
make
```

The example builds and executable `example_glfw_opengl3.exe` is
created.

Open a Vim terminal (or another `mingw` shell) and run it:

```
./example_glfw_opengl3.exe 
```

# Vim setup to navigate the IMGUI repo

Without going crazy making a tags file and a cscope database, Vim
`:find` and `:vimgrep` are good enough to navigate the IMGUI
repo.

- `:find` handles jumping to header files
- `:vimgrep` makes it easy to find the line number of the
  function signature

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
