# Quicker Chats
Quicker Chats is a little program for making custom quick chats in [Rocket League](https://www.rocketleague.com/).  
It's a linux specific tool that uses X11 for hooking and sending keypresses.

Quicker Chats also records which keys are being held and will restore the keyboard's state after sending the quick chat. This allows for seamless quickchats without screwing with the players ingame movement.

### Quick Start
Make sure you have `libxi-dev` installed.
```shell
git clone https://github.com/kregerl/quicker-chats.git
cd quicker-chats
cmake CMakeLists.txt && make
./quicker_chats
```

###Usage
```shell
USAGE: quickerchat [OPTIONS] FILE
  FILE should be a mappings file for mapping keys to strings.
  OPTIONS
    -id    The device id to use, from xinput
```
### Mappings file
The mappings file is used for creating keybindings and the messages they will send.  
Quicker Chats expects the mappings file to be in a key-value format seperated by colons.
To define a keybinding you must use the X11 keysym name with the `XK` prefix removed. For example `XK_KP_0` should be written as `KP_0`.  

All X11 keysyms can be found at `/usr/include/X11/keysymdef.h` or [here](https://www.cl.cam.ac.uk/~mgk25/ucs/keysymdef.h)
Please see `example_mappings.txt` for an example mappings file.