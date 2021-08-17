#include <stdio.h>
#include <err.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "apple_invaders.h"

#define TYPE_BYTE 0xef

#define PAUSE_BYTE 0x01
#define DATA_BYTE 0x02
#define DONE_BYTE 0x03

/* Format is <TYPE_BYTE> (type) (two byte length, minus header) (optional data) */
/* Length, other fields are all little endian */

/*
 * Do a quick hack to output a little TLV wrapper for
 * a game to see if I can get the damned thing to load an run right.
 */
int
main(int argc, const char *argv[])
{
  int size = sizeof(apple_invaders_bin);
  int fd;
  uint8_t buf[8];

  fd = open("apple_invaders.bin", O_CREAT | O_RDWR | O_TRUNC);
  if (fd < 0)
    err(1, "open");

  printf("size: %u\n", size);

  (void) lseek(fd, 0, SEEK_SET);

  /* Data - two byte size, header period, pre-blank, post-blank, followed by the data */
  buf[0] = TYPE_BYTE;
  buf[1] = DATA_BYTE;
  buf[2] = (size + 3) & 0xff;
  buf[3] = ((size + 3) >> 8) & 0xff;
  // And now the data - including our custom header
  buf[4] = 31; // Periods
  buf[5] = 10; // Pre-blank
  buf[6] = 10; // Post-blank
  write(fd, buf, 7);

  write(fd, apple_invaders_bin, size);

  /* Done */
  buf[0] = TYPE_BYTE;
  buf[1] = DONE_BYTE;
  buf[2] = 0;
  buf[3] = 0;
  write(fd, buf, 4);

  /* Wrap it up */
  close(fd);

  exit(0);
}
