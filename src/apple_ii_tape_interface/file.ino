#include <SdFat.h>

#include "config.h"

SdFat sd;
SdFile entry;

void
file_setup(void)
{
  pinMode(SD_CARD_PIN_CS, OUTPUT);
  if (! sd.begin(SD_CARD_PIN_CS, SPI_FULL_SPEED)) {
    Serial.println("SD: None found.");
    return;
  }
  // Set SD to root directory
  sd.chdir();
  Serial.println("SD: Init done.");
}

void
file_open(const char *file)
{
  entry.open(file, O_RDONLY);
}

void
file_close(void)
{
  entry.close();
}

int
file_read_bytes(unsigned char *buf, int len)
{
  int i;
  i = entry.read(buf, len);
  return i;
}

uint64_t
file_get_size(void)
{
  return entry.fileSize();
}

int
file_seek_start(void)
{
  entry.seekSet(0);
  return 0;
}
