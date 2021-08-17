#include <stdio.h>
#include <err.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_BYTE 0xef

#define PAUSE_BYTE 0x01
#define DATA_BYTE 0x02
#define DONE_BYTE 0x03

/* Format is <TYPE_BYTE> (type) (two byte length, minus header) (optional data) */
/* Length, other fields are all little endian */

struct mon_load {
  int start_addr;
  int end_addr;
  int len;
};

/*
 * Parse given monitor file into a buffer.
 * It'll parse up to 64k of data.
 */
static int
mon_file_parse(const char *file, uint8_t *buf, const int len, struct mon_load *ml)
{
  int start_addr = -1, end_addr = -1, addr = -1, offset = 0;
  FILE *fp;

  fp = fopen(file, "r");
  if (fp == NULL) {
    warn("fopen");
    return (1);
  }

  while (!feof(fp)) {
    char parse_buf[256];
    char *p, *e;

    p = fgets(parse_buf, 255, fp);
    if (p == NULL) {
      break;
    }
    if (p[0] == '\0')
      continue;

    /* Trim off any trailing \r\n */
    e = p + (strlen(p)-1);
    while (e != p && (*e == '\r' || *e == '\n')) {
      *e = '\0';
      e--;
    }

    //printf("parsing: '%s'\n", p);

    /* Parse an address field */
    e = strsep(&p, ":");
    if (e == NULL) {
      break;
    }

    addr = strtoul(e, NULL, 16);
    if (start_addr == -1) {
      start_addr = addr;
    } else {
      // XXX Validate that this address is contiguous!
    }
    //printf("address: 0x%x, offset 0x%x\n", addr, offset);

    /* Strip any following spaces */
    while (*p == ' ' || *p == '\t') {
      p++;
    }

    /* Loop over and parse hex digits */
    end_addr = addr;
    while ((e = strsep(&p, " ")) != NULL) {
      uint8_t val = strtoul(e, NULL, 16);
      //printf(" '0x%x'", val);
      buf[offset] = val;
      offset++;
      end_addr++;
    }
    //printf("\n--\n");
  }

  fclose(fp);
  ml->start_addr = start_addr;
  ml->end_addr = end_addr - 1;
  ml->len = offset;
  return (1);
}

/*
 * Do a quick hack to output a little TLV wrapper for
 * a game to see if I can get the damned thing to load an run right.
 */
int
main(int argc, const char *argv[])
{
  int fd;
  uint8_t buf[8];

  struct mon_load ml;
  uint8_t *mon_buf;
  int mon_len;

  mon_buf = calloc(65536, sizeof(char));
  mon_len = 65536;

  if (mon_file_parse(argv[1], mon_buf, mon_len, &ml) != 1) {
    exit(1);
  }

  fd = open(argv[2], O_CREAT | O_RDWR | O_TRUNC);
  if (fd < 0)
    err(1, "open (create)");

  (void) lseek(fd, 0, SEEK_SET);

  /* Data - two byte size, header period, pre-blank, post-blank, followed by the data */
  buf[0] = TYPE_BYTE;
  buf[1] = DATA_BYTE;
  buf[2] = (ml.len + 3) & 0xff;
  buf[3] = ((ml.len + 3) >> 8) & 0xff;
  // And now the data - including our custom header
  buf[4] = 31; // Periods
  buf[5] = 10; // Pre-blank
  buf[6] = 10; // Post-blank
  write(fd, buf, 7);

  write(fd, mon_buf, ml.len);

  /* Done */
  buf[0] = TYPE_BYTE;
  buf[1] = DONE_BYTE;
  buf[2] = 0;
  buf[3] = 0;
  write(fd, buf, 4);

  /* Wrap it up */
  close(fd);

  printf("Wrote %d bytes, load with %X.%XR %XG\n",
    ml.len,
    ml.start_addr,
    ml.end_addr,
    ml.start_addr);

  exit(0);
}
