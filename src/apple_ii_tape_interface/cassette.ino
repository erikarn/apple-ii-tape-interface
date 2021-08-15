#include "config.h"
#include "cassette.h"

// Blocking cassette routines!

// Write out a header
// each period here is 128 * 1300uS, so 166mS.
void
cassette_header(unsigned short periods)
{
  // Header tone - 770Hz
  for (int i = 0; i < periods*128; i++) {
    digitalWrite(SPEAKER_PIN, HIGH);
    delayMicroseconds(650);
    digitalWrite(SPEAKER_PIN, LOW);
    delayMicroseconds(650);
  }

  // Sync pulse; one half cycle at 2500hz and then 2000hz
  digitalWrite(SPEAKER_PIN, HIGH);
  delayMicroseconds(200);
  digitalWrite(SPEAKER_PIN, LOW);
  delayMicroseconds(250);
}

void
cassette_write_byte(unsigned char val)
{
  //Serial.print(val); Serial.print(":");
  for (unsigned char i = 8; i != 0; --i) {
    digitalWrite(SPEAKER_PIN, HIGH);
    delayMicroseconds((val & _BV(i-1)) ? 500 : 250);
    digitalWrite(SPEAKER_PIN, LOW);
    delayMicroseconds((val & _BV(i-1)) ? 500 : 250);
  }
}

void
cassette_write_block_progmem(const uint8_t *ptr, unsigned short len)
{
  unsigned char checksum = 0xff, val = 0;
  for (unsigned short i = 0; i < len; i++) {
    val = pgm_read_byte(ptr + i);
    cassette_write_byte(val);
    checksum ^= val;
  }
  cassette_write_byte(checksum);
  digitalWrite(SPEAKER_PIN, HIGH);
  delay(10); // Yes, 10ms
  digitalWrite(SPEAKER_PIN, LOW);
  
}

void
cassette_setup(void)
{
  Serial.print("Cassette setup\n");
  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, LOW);
}
