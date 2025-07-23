![https://github.com/pvini07BR/3dsky/tree](https://github.com/pvini07BR/3dsky/blob/main/gfx/3dsky-icon-alt.svg)

# WARNING: This app is still in development. It doesn't do much yet. Expect bugs, crashes and lack of optimizations.

In order to run this app, you will need a SSL certificate file. But don't worry, the app itself checks if you have that file, and if you don't, it will download it for you.

# Roadmap

What this Bluesky client is currently able to do:

✅ Login

✅ Load timeline posts

✅ Load profile info and posts

🟩 View post and its replies **(working on it)**

⬜ Interact with posts (like, comment, repost)

⬜ View post embedded image(s)

⬜ Create new posts

⬜ View different feeds

⬜ View post embedded video(s)

⬜ Search

⬜ Chat

⬜ Display emojis

# Known issues

Currently, links or very long words cannot be wrapped on the posts. Unfortunately, this is a Clay issue, not an issue from the program, so there isn't much I can do about it.

There is already an open issue about it: https://github.com/nicbarker/clay/issues/165

# How to compile

**WARNING:** The compilation will not work if there is spaces on your directory, such as your username. Make sure the directory to this project has no spaces, otherwise it won't work.

You will need to have [devkitPro](https://devkitpro.org/wiki/Getting_Started) installed.

After having devkitPro set-up, you will need to install the following packages:
```
(dkp-)pacman -S 3ds-dev 3ds-jansson 3ds-curl
```

And to compile the program, all you need to do is run the command ``make``. It will compile
a ``.3dsx`` file, which is the file that will be ran on your 3DS or in a emulator.

You can send the compiled .3dsx file into your Nintendo 3DS by opening Homebrew Launcher, pressing Y,
and then running this command on the terminal:
```
3dslink -a [your 3DS IP address] 3DSky.3dsx
```

Currently there is no way of compiling to a .cia file yet, as the program isn't finished yet, and due to
the ever-changing nature of the project. I plan on adding compiling to .cia when the program is more or less ready.

# Auto completion support

If you're going to use clangd for auto completion, you will need the ``compile_commands.json`` file
to get clangd autocompletion working. To generate that file, you will need the ``bear`` command available on
your system. Generate it by running ``bear -- make``. Running this command will both generate the compile commands
file and compile the program. You only need to run this command once to generate the compile commands file. After
that, you can just run ``make`` to compile the project normally.

Also, this repo comes with a .clangd file, which helps you to get clangd auto completion working.
However, it's paths are set for an Linux enviroment only. If you're on Windows or any other OS,
all you need to do is change the paths (I wish there was a way of setting the paths depending on the OS though).

But to get the auto completion fully working, you need the ``compile_commands.json`` file (as stated above).

There is also a c_cpp_properties.json file for VSCode users with the C/C++ Language Extension.
It only supports Windows with MSYS2 for now though.

# Debugging

On the beginning of main.c, there is a ConsoleMode variable. You can change its value to toggle between
console on the top screen, or bottom screen, or neither.

You can also provide your Bluesky login credentials to the Make arguments to automatically login every time
you launch up the program. (I plan on adding a way of saving the login credentials into a file in the future though)

To do that, you compile with the following command:
```
make LOGIN_HANDLE=user.bsky.social LOGIN_PASSWORD=yourpasswordhere
```

NOTE: If you have compiled a .3dsx file before, you may have to delete that previous file before compiling again,
so it will take the effects of the login credentials. It's recommended to run ``make clean`` to remove any previous
compilation artifacts, and compile from scratch.

This project also comes with a ``launch.json`` file to do remote debugging with your 3DS. But for that, you will need
the [Native Debug extension](https://marketplace.visualstudio.com/items?itemName=webfreak.debug).

However, this file needs to be modified in order to fully work in your enviroment.

In the file, change the IP address and port to your 3DS. The port usually is the same: 4003.
Also change the GDB debugger path if you are in a non-linux enviroment, or if devkitPro is installed
somewhere else.

Also, this file has some hardcoded paths for the source codes of [citro2D](https://github.com/devkitPro/citro2d), [citro3D](https://github.com/devkitPro/citro3d) and [libctru](https://github.com/devkitPro/libctru). You need to change them if you want the
debugger to automatically go to the source files related to any of these three libraries.

To start debugging, first launch Homebrew Launcher on your 3DS. Then open the Rosalina menu (L+down+Select),
go to "Debugger options", then "Enable debugger", and then "Force-debug next application at launch".
Then you either launch the ``3DSky.3dsx`` file already present on your device, or send it with ``3dslink``.
After launching the program, you will encounter a black screen. This is the intended behavior. Now
go on the "Run and Debug" option in VSCode and click the play button above, with the label "Attach to gdb server".
The program will automatically run again as normally, but the GDB will be attached.

# Credits

Icons used in this project has been taken from [Flaticon](https://www.flaticon.com).

Special thanks to everyone who helped me on the [Nintendo Homebrew Discord Server](https://github.com/nh-server)
on the #dev channel, to make this project possible.

Libraries used:
- [curl](https://github.com/curl/curl)
- [Clay](https://github.com/nicbarker/clay)
- [Jansson](https://github.com/akheron/jansson)
- [libbluesky](https://github.com/briandowns/libbluesky) (Had to be modified to accomodate this project's needs.)
- [uthash](https://github.com/troydhanson/uthash)
