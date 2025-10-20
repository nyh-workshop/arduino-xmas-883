# arduino-xmas-883
An improvisation of Myke Predko's "xmas" project in "Programming and Customizing the PIC Microcontroller 3rd Edition"

*Note*: This has been attempted in my other [repository](https://github.com/nyh-workshop/arduino-xmas-883-2023) too in 2023. However this one is a more detailed version and there are example STL and Gerber files for the materials.

![](/images/xmas-883-new.jpg)

## Introduction
About some 17 years back when I was in college, I chanced upon the Myke Predko's *Programming and Customizing the PIC Microcontroller* book in a local bookstore when I wanted to find study materials on microcontroller subject next semester.

It was extremely expensive over there in the place I stay and I could only glance a few pages of the book for previewing.

While admiring the projects in the book I saw one of the projects in page 883 called `xmas`, caught my attention and tried to remember as much details as possible.

Little me didn't know I could make things like that with a PIC microcontroller! Since I had a lot of space constraints at home, having a miniature, singing Christmas tree would be so cool on *my very own desk*!

I had attempted variants of this one almost each year - each of them are different every season, from one WAV file player, to another with different amount of LEDs, and with different microcontrollers from PIC32 to even ESP32. That too polished up some of the embedded skills of course!

## Details of the original `xmas`
### Random blinky lights
- From the schematic the author uses a `PIC16F84` microcontroller, coupled with a few `74LS373` shift registers to expand the I/O to accomodate those 16 LEDs.

- There is a pseudo-random number generator written to generate a random LED pattern, which is then manually bit-banged into the `74LS373`.

- The author mentioned about *wiring these 16 LEDs one-by-one* to the back of the tree, with many wires being used. Yes, it is tedious!

### Music
- It is a square-wave, monophonic. Bit-banged through a GPIO pin.

- The tone generation and reading the "notes" in the array is using a timer interrupt, possibly a `TMR0` one.

- The author recommends to use a beginner's Christmas music piano book to put these notes into the array manually. It has to be with the certain frequency range.

### Construction
- The original one uses **wood pieces and some paint**. Basically, the triangle one uses green and the base could be a brown to simulate the wood color.

- The PCB is manually sketched and etched locally. It is attached to the back of the tree "trunk" (*see diagram*).

![](/images/xmas-883-original.png)

### Power
- It is powered through the DC jack, with the casual 7805.

## Details of the improvised `xmas` in 2025
### Random blinky lights (and more colorful LEDs!)
- Using the [WS2811/WS2812 addressable LEDs](https://cdn-shop.adafruit.com/datasheets/WS2812.pdf). Saves me the time from wiring the LEDs and also these LEDs can display any 24-bit RGB color. Plus, many libraries such as [FastLED](https://fastled.io/) offers support for these LEDs without having to rewrite special LED effects again.

- Instead of a PIC microcontroller, a [Raspberry Pico 2](https://www.raspberrypi.com/products/raspberry-pi-pico-2/) is used instead with the [Arduino Pico library](https://github.com/earlephilhower/arduino-pico). There is PIO (Programmable I/O) modules that helps to generate WS2812 signals without CPU intervention.

- This blinky activity is entirely run on core0 of this microcontroller.

- Soldering them on a PCB (Gerber included here) is even easier. It's all on the board!

### Music
- This uses a [modified MicroDexed library](https://codeberg.org/nyh-workshop/MicroDexed) for the Raspberry Pico 2 Arduino platform. Basically it is a Yamaha DX7 emulator. This generates the popular FM soundwaves that you hear from the 80s music.

- And, of course we need the music notes inside! I have the [modified MD_MIDIFile that reads the MIDI from the Raspberry Pico 2's flash memory](https://github.com/nyh-workshop/MD_MIDIFile_LittleFS). This saves me the extra SD card that I might use later. There is an Arduino plugin that [uploads the files into the Pico 2's flash memory](https://github.com/earlephilhower/arduino-pico-littlefs-plugin) too.

- Best of all it covers most of the frequency of the music notes!

- It is output through the [MAX98357 module](https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/overview), with a small speaker.

- The music playing and the emulator is running on another core1 of this microcontroller.

### Power
- It is powered through the regular micro-USB.

### Construction
- The original one was a bit too big for my desk or on my shelves, so I made the tree smaller.

- It has a multi-purpose PCB mainboard (check the Hardware folder) where I can mount the Pico 2, the MAX98357 module, the optional [M5 Unit Synth](https://docs.m5stack.com/en/unit/Unit-Synth) connector and the WS2811 port there. Gerbers are included if you want to send it to popular PCB manufacturers.

![](/images/xmas-883-new-PCB.png)

- The WS2812 LEDs are mounted on the triangle portion of the tree. It is also a PCB for convenience sake.

![](/images/xmas-883-new-tree.png)

#### 3D printed tree and body

Since I can't *easily* source any wood pieces right now (or even before), it would be much easier if I just 3D-print it.

Similarly, I used the 3D-print `forest green` filament for the tree part (triangle), and the rest of the base and the trunk with `brown` filament.

I drew the parts using **Fusion 360**. It consists of the box that holds the mainboard box, its base, the trunk, and finally the tree part:

![](/images/xmas-883-new_front.png)

From the original, mounting the PCB at the back of the tree is possibly impractical due to the location of the USB port. So, instead of putting it at the back, I choose to put it into the box, with the speaker glued to the front of the box.

Here is the construction when the board is fitted onto the box tray, with kapton tapes securing the wires:
![](/images/xmas-883-new-mainboard-photo.jpg)

To fit the PCB tree part to the 3D-print tree part (the triangle one), it is recommended that you use a double-sided tape to hold these together firmly.

There's a small rectangular hole in front of the box - it is slightly to the right. It is for the wires for the **small rectangular speaker (8 ohms, 2 watts)** and these are connected to the MAX98357 module's output. Again, use a double-sided tape to mount the speaker in front of the box.

### Software
For the LED part, the "blinky" is more than just blink now - there are all the colors in the world! I have used the FastLED example `Demoreel 100` and modified with some extra effects such as twinkling and such.

You can get the Christmas MIDIs from online, and then put these into the `data` folder before doing the `Upload LittleFS to Pico`. Make sure you have select the `Flash Size 128KB` or more at the `Tools` section.

For better playability of the MIDI file it is recommended that:
- Only one track is used. Multitrack may not work well on it.
- Keep the voices around 11 maximum.
- It does not play multiple instruments - only one is played. This will closely "imitate" the original design which is only a square-wave.

To modify the MIDI files back to one track and other things to accomodate the player, apps such as [MidiEditor](http://www.midieditor.org/) can be used.

As the player can support different instrument sounds, I have programmed it to play a different instrument every each time you switch it on. 

### Issues
- Since the MicroDexed is pretty CPU-intensive, I do expect a small amount of stuttering during playback. On testing, sometimes the MIDI player **behaved erratically** and have **too many notes ringing** at once. This causes the playback to stutter and eventually sounding off-key in a terrible manner. To minimize that unusual stuttering and stalling, I have enabled the `STUTTER_REBOOT` to **force reboot** the system when this happened. If it is disabled, the core1 will **halt** there while the LED blinks at the fastest way possible.
