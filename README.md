# WARNING: This app is still in development. It doesn't do much yet.

Also, to be able to run it and make HTTPS requests, you will need a SSL certificate file. It's a file called ``cacert.pem``, and it should be under the ``config/ssl`` directory on the 3DS' SD card.

# How to compile

This repo has been made on Linux, more specifically Arch Linux.
You will need to have [devkitPro](https://devkitpro.org/wiki/Getting_Started) installed.
You will need these packages:
```
sudo (dkp-)pacman -S 3ds-dev 3ds-jansson 3ds-curl
```

After that, all you have to do is run ``make`` on the project directory, and it should compile into a .3dsx file.
You can send it into your Nintendo 3DS by opening Homebrew Launcher, pressing Y, and then running ``3dslink -a [address] [compiled file].3dsx`` on your terminal.
