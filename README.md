# ArduDisco
"Disco lights" using an Arduino (Uno in my case) or two of them, a NeoPixel compatible LED strip and a IR remote.

# How?
I managed to get it work using a "dirty hack" for a single Arduino setup, you have to press your IR button twice though.
The dual Arduino setup should be more reliable and without any twice-clicking required.

# Can I use it?
If you want to use it - do it. If you don't, then don't.
Change everything as you might need to get it work.


If you have any ideas on how to improove my code i'd really appreciate your ideas. :)


# Credits
The main code for the "disco" effects is forked from https://github.com/bartlettmic/SparkFun-RGB-LED-Music-Sound-Visualizer-Arduino-Code.
I used cyborg5's IRLib2 as infrared recieving library from https://github.com/cyborg5/IRLib2 and Adafruit_NeoPixel as LED library which can be found at https://github.com/adafruit/Adafruit_NeoPixel.