#include <SdFat.h>

#include "config.h"

SdFat sd;
SdFile entry;
SdFile dirFile;

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

/*
 * Get a count of the number of files in a directory.
 */
int
dir_get_file_count(void)
{
  int i = 0;

  if (!dirFile.open("/", O_RDONLY)) {
    Serial.println("ERROR: can't open the directory");
    return -1;
  }

  while (entry.openNext(&dirFile, O_RDONLY)) {
    if (entry.isSubDir() || entry.isHidden())
      goto next;
    if (! entry.isFile())
      goto next;
    i++;
  next:
    entry.close();
  }
  dirFile.close();
  return i;
}

/*
 * Get a file by index.
 * 
 * This gets the filename and also leaves the file open in case we
 * wish to read from it.
 */
int
dir_get_file_by_index(int index, char *filename, int len)
{
  int i = 0;

  if (!dirFile.open("/", O_RDONLY)) {
    Serial.println("ERROR: can't open the directory");
    return -1;
  }
  
  entry.close();
  
  while (entry.openNext(&dirFile, O_RDONLY)) {
    if (entry.isSubDir() || entry.isHidden())
      goto next;
    if (! entry.isFile())
      goto next;

    if (i == index) {
      /* Copy out as much of the filename as we can */
      entry.getName(filename, len);
      /* Leave the file open in case this is what we're opening */
      //entry.close();
      dirFile.close();
      return 1;
    }
    i++;
  next:
    entry.close();
  }
  dirFile.close();
  return 0;
}

/*
 * Rewind to the beginning of the file.
 */
bool
file_seek_beginning(void)
{
  return entry.seekSet(0);
}
