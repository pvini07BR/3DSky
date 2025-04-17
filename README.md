# WARNING: To be able to run this app, you need a SSL certificate on your 3DS.

# How to compile

This repo has been made on Linux, more specifically Arch Linux.
You will need to have [devkitPro](https://devkitpro.org/wiki/Getting_Started) installed.
You will need these packages:
```
sudo (dkp-)pacman -S 3ds-dev 3ds-jansson 3ds-curl
```

After that, all you have to do is run ``make`` on the project directory, and it should compile into a .3dsx file.
You can send it into your Nintendo 3DS by opening Homebrew Launcher, pressing Y, and then running ``3dslink -a [address] [compiled file].3dsx`` on your terminal.
