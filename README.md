
## Introduction

This is an implementation of a digital cassette playback device
for the apple-ii.

It implements a simple binary block protocol to wrap up data
dumps of cassette data and generates the audio necessary to
play it back to the apple-ii.

Yes, instead of storing .wav files, this just stores binary
data dumps, a few tens of kilobytes each.

## How it works

This is inspired by the TZXDuino project!

The audio itself is generated inside the Timer1 ISR.
It handles generating the header, sync and data audio patterns.
They're all under 2KHz so it's not a huge stretch to do it, but
it does require some pretty tight timing.

The data files are stored in a binary TLV (type length value)
format.  The TLV headers describe the data itself, how long
the header tone should be, and any pre and post playback
pauses needed before a subsequent block.

Since the apple-ii doesn't have its own header binary format
like the Amstrad CPC and Commodore 64 does, the user is required
to provide load addresses - or it needs to be a BASIC program.
BASIC records two data records - one as a short header describing
the subsequent data, and then the subsequent data.

The UI is four push buttons and a 16x2 i2c connected LCD.
Data is stored on a FAT formatted SD card.  This currently doesn't
support directories, so stick your files in the root directory
of the flash drive.

## Wiring

I'll eventually make a schematic.  But honestly, it's pretty simple.
The pins themselves are described in config.h !

 * An arduino nano
 * D2 is the speaker output - put a 3.9k resistor between it and
   the audio output jack, and then a 1k resistor between the audio jack
   and ground. This 3.9k and 1k resistor forms a voltag divider that
   should deliver around 1v peak to peak of audio.
 * The four buttons (PLAY, STOP, PREV, NEXT) are wired to ground.
   The software configures the input pins with a pull-up.
   Wire D3 to PLAY, D4 to STOP, D5 to PREV, D6 to NEXT.
 * The SD card is an Adafruit SPI SD card - MOSI on pin 11, MISO on pin 12,
   SCK on pin 13, CS/SS on pin 10.  No card detect is currently used.
 * The i2c 16x2 LCD hooks up to the i2c pins - SDA on A4, SCL on A5.

And that's it!

## Controls

The four buttons are:

 * PLAY - select a tape / display its load info; a second button
   press will begin playback.
 * STOP - stop an active playback.
 * PREV - select the previous tape.
 * NEXT - select the next tape.

## Data format

I'll document the data format in docs/tape_format.txt.

## Conversion tool

The cnv tool is in src/cnv/ . It's a simple C program.
