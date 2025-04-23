![https://github.com/pvini07BR/3dsky/tree](https://github.com/pvini07BR/3dsky/blob/main/gfx/3dsky-icon-alt.svg)

# WARNING: This app is still in development. It doesn't do much yet.

In order to run this app, you will need a SSL certificate file. But don't worry, the app itself checks if you have that file, and if you don't, it will download it for you.

# Known issues

Currently, links or very long words cannot be wrapped on the posts. Unfortunately, this is a Clay issue, not an issue from the program, so there isn't much I can do about it.

There is already an open issue about it: https://github.com/nicbarker/clay/issues/165

# How to compile

You will need to have [devkitPro](https://devkitpro.org/wiki/Getting_Started) installed.

You will need these packages:

```
(dkp-)pacman -S 3ds-dev 3ds-jansson 3ds-curl
```

After that, all you have to do is run ``make`` on the project directory, and it should compile into a ``3dsky.3dsx`` file.

You can send it into your Nintendo 3DS by opening Homebrew Launcher, pressing Y, and then running on the terminal:
```
3dslink -a [your 3DS IP address] 3dsky.3dsx
```

Currently there is no way of compiling to a .cia file yet, as the program isn't finished yet. I plan on adding compiling to .cia when its more or less ready.

Also, this repo comes with a .clangd file, which helps you to get code auto completion working.
However, it's paths are set for an Linux enviroment only. If you're on Windows or any other OS, all you need to do is change the paths.

(I wish there was a way of setting the paths depending on the OS though)

Also, you need the ``compile_commands.json`` file, which you can generate by running ``bear -- make`` (make sure you have ``bear`` installed on your system though). The reason why it haven't been added in the repo is because it depends on how the program gets compiled.

# Credits

Libraries used:
- [curl](https://github.com/curl/curl)
- [Clay](https://github.com/nicbarker/clay)
- [Jansson](https://github.com/akheron/jansson)
- [libbluesky](https://github.com/briandowns/libbluesky)
