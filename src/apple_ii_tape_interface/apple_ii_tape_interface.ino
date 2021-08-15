#include "config.h"

#include "cassette.h"
#include "buttons.h"

//#include "apple_invaders_disp.h"

// quick hack - for now, manually need to load this at 0x3fd; I'll look
// into a c2d style loader wrapper later

static const PROGMEM uint8_t apple_invaders_bin[] = { 0x1, 0x2, 0x3, 0x4 };

void setup() {
  unsigned char checksum = 0xff;
  long file_size;
  
  Serial.begin(9600);
  display_setup();
  cassette_setup();
  cassette_new_init();
  buttons_setup();
  display_clear();
  file_setup();

  /* 
   * XXX TODO - ideally we'd only be doing this once we've parsed the 
   * data file and know how long of a header we need to provide!
   */
  new_cassette_data_init();
  new_cassette_period_length_set(31); // ~ 10 seconds
  new_cassette_period_set_pre_blank(10);
  new_cassette_period_set_post_blank(10);

  // For now, pre-load a single file - apple_invaders.bin
  // to load/run in monitor - 3FD.537CR 3FDG
  file_open("apple_invaders.bin");
  file_size = file_get_size();
  new_cassette_data_set_length(file_size + 1L); // Include the checksum byte!

  // Start the cassette playback with the above info
  cassette_new_start();

  int i, j = 0;
  unsigned char buf[1];

  // Start immediately filling the FIFO with data!
  while (file_read_bytes(buf, 1) == 1) {
    //Serial.print(buf[0] & 0xff, HEX);
    while (new_cassette_data_add_byte(buf[0]) != true) {
      delay(1);
    }
    checksum ^= buf[0];
    
    j++;
    if (j == 128) {
          Serial.print(".");
          j = 0;
    }
  }

  // Send "checksum" byte
  while (new_cassette_data_add_byte(checksum) != true) {
    delay(1);
  }

  file_close();
  Serial.println("\n--\nDone.");

  // Now wait until the state is NONE
  while (cassette_new_get_state() != 0) {
    delay(1);
  }
  Serial.println("--\nFinished sending.");
}

void
send_file_blocking(void)
{
    Serial.print("Starting send\n");
    Serial.print("File size: ");
    Serial.print(sizeof(apple_invaders_bin));
    Serial.print("\n");
    Serial.flush();

    // Disable interrupts during sending audio
    noInterrupts();

    // 128 periods before data block
    cassette_header(128);

    // For now our only data block is the program we've hard coded, it needs
    // to be loaded at 0x03fd.
    //
    // So at the monitor (not BASIC!), type:
    //
    // 3FD.537CR 3FDG
    //
    cassette_write_block_progmem(apple_invaders_bin, sizeof(apple_invaders_bin));

    // Re-enable interrupts
    interrupts();
    
    Serial.print("Done.\n");
    Serial.flush();
}
void loop() {
}
