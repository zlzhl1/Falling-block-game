# TTGO Demo
This is a demo project for the TTGO T-Display Board used in 159.236 at Massey.
It uses PlatformIO, the esp-idf and my own simple graphics library.
It displays a menu with a rotating teapot that lets you run some demos.

To try it out, install Vscode, add the the PlatformIO IDE extension, open the TTGODemo folder and click 
the arrow -> in the status bar (PlatformIO:Upload).

There are 2 environments defined in the platformio.ini file,
an emulator(env:emulator) and a real board (env:tdisplay). 
The default is use the emulator but you can upload to a real board by setting the environment
in the platformio status bar to env:tdisplay.

The emulator should work on Windows, Linux and OSX

This is what it looks like in the emulator:

![ttgodemo](https://github.com/a159x36/TTGODemo/assets/53783/c8e037c2-7b99-41db-97b1-4945b738eee4)
