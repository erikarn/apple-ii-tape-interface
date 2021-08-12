
#include "apple_invaders.h"

// Speaker on D2
#define SPEAKER_PIN 2

// quick hack - for now, manually need to load this at 0x3fd; I'll look
// into a c2d style loader wrapper later


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

void setup() {
  Serial.begin(9600);
  cassette_setup();
}

void loop() {
  static char do_output = 0;
 
  // put your main code here, to run repeatedly:
  if (do_output == 0) {
    Serial.print("Starting send\n");
    Serial.print("File size: ");
    Serial.print(sizeof(apple_invaders_bin));
    Serial.print("\n");
    do_output = 1;

    // 128 periods before data block
    noInterrupts();
    cassette_header(128);

    // For now our only data block is the program we've hard coded, it needs
    // to be loaded at 0x03fd.
    //
    // So at the monitor (not BASIC!), type:
    //
    // 3FD.537CR 3FDG
    //
    cassette_write_block_progmem(apple_invaders_bin, sizeof(apple_invaders_bin));
    interrupts();
    Serial.print("Done.\n");
  }

}
