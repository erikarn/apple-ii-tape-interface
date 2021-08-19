# cnv

cnv is a simple program that takes a .mon file from the apple game server online
database and will turn it into a tape image.

The .mon file is a hexdump of the tape data suitable to feed into c2t to
generate apple-ii audio wav files.

## to compile

cc cnv.c -o cnv

## to use

./cnv game.mon game.bin

Then place the .bin file in your tape SD card.
