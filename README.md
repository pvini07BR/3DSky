![https://github.com/pvini07BR/3dsky/tree](https://github.com/pvini07BR/3dsky/blob/main/gfx/3dsky-icon-alt.svg)

# WARNING: This app is still in development. It doesn't do much yet. Also expect bugs and crashes.

In order to run this app, you will need a SSL certificate file. But don't worry, the app itself checks if you have that file, and if you don't, it will download it for you.

# Known issues

Currently, links or very long words cannot be wrapped on the posts. Unfortunately, this is a Clay issue, not an issue from the program, so there isn't much I can do about it.

There is already an open issue about it: https://github.com/nicbarker/clay/issues/165

# How to compile

**WARNING:** The compilation will not work if there is spaces on your directory, such as your username. Make sure the directory to this project has no spaces, otherwise it won't work.

You will need to have [devkitPro](https://devkitpro.org/wiki/Getting_Started) installed.

You will need these packages:

```
(dkp-)pacman -S 3ds-dev 3ds-jansson 3ds-curl
```

Also if you're going to use clangd for auto completion, you might need the ``compile_commands.json`` file
to get clangd autocompletion working. Run ``bear -- make`` to generate the file (make sure you have ``bear``
installed on your system though). The reason why it haven't been added in the repo is because it depends on
how the program gets compiled.

By running ``bear -- make``, you will compile the program and generate ``compile_commands.json`` at the same time.
This is the recommended approach. You only need to run it once though, then you just run ``make``.

You can send the compiled file into your Nintendo 3DS by opening Homebrew Launcher, pressing Y,
and then running on the terminal:
```
3dslink -a [your 3DS IP address] 3DSky.3dsx
```

Currently there is no way of compiling to a .cia file yet, as the program isn't finished yet.
I plan on adding compiling to .cia when its more or less ready.

Also, this repo comes with a .clangd file, which helps you to get clangd auto completion working.
However, it's paths are set for an Linux enviroment only. If you're on Windows or any other OS,
all you need to do is change the paths (I wish there was a way of setting the paths depending on the OS though).

But to get the auto completion fully working, you need the ``compile_commands.json`` file (as stated above).

There is also a c_cpp_properties.json file for VSCode users with the C/C++ Language Extension.
It only supports Windows with MSYS2 for now though.

# Debugging

On the beginning of main.c, there is a ConsoleMode variable. You can change its value to toggle between
console on the top screen, or bottom screen, or neither.

You can also provide your Bluesky login credentials to the Make arguments and go straight to the main scene,
instead of having to go through the login scene everytime.

To do that, you compile with the following command:
```
make LOGIN_HANDLE=user.bsky.social LOGIN_PASSWORD=yourpasswordhere
```

NOTE: If you have compiled a .3dsx file before, you may have to delete that previous file before compiling again,
so it will take the effects of the login credentials. It's recommended to run this command to clean all previously
compiled files:

```
make clean
```

# Credits

Special thanks to everyone who helped me on the [Nintendo Homebrew Discord Server](https://github.com/nh-server)
on the #dev channel, to make this project possible.

Libraries used:
- [curl](https://github.com/curl/curl)
- [Clay](https://github.com/nicbarker/clay)
- [Jansson](https://github.com/akheron/jansson)
- [libbluesky](https://github.com/briandowns/libbluesky) (Had to be modified to accomodate this project's needs.)
- [uthash](https://github.com/troydhanson/uthash)
