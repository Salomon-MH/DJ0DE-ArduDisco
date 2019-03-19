# ArduDisco - [DJ0.DE](https://dj0.de) (Open Source Version)
"Disco lights" using an Arduino (Nano in my case) or two of them, a NeoPixel compatible LED strip and a IR remote.
This is the open source version and does not necessarily represent all or any features & code structure at all of the private version of DJ0DE. Feel free to contribute new features though, of course I'll push these to the open source version. It is not clear if the private version of DJ0DE will be published anytime soon.

# How?
I managed to get it work using a "dirty hack" for a single Arduino setup, you have to press your IR button twice though.
The dual Arduino setup should be more reliable and without any twice-clicking required - that's why I used it in my personal setup. I wont provide updates for the single setup too.

# Can I use it?
Please ask for permission first and always name the source.
Especially distributing the software as your own or using it for commercial purposes is strongly forbidden.
This **does** apply on the open source version too.

If you have any ideas on how to improve my code I'd really appreciate your ideas. :)


# Credits
I used [cyborg5's IRLib2](https://github.com/cyborg5/IRLib2) as infrared recieving library and [Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) as LED library.
The basic code for some of the "disco" effects is forked from [SparkFun's RGB LED Music Sound Visualizer Arduino Code](https://github.com/bartlettmic/SparkFun-RGB-LED-Music-Sound-Visualizer-Arduino-Code).
