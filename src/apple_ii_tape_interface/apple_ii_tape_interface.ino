#include "config.h"

#include "buttons.h"

// No active tape
#define PLAY_STATE_NONE 0

// Active tape states - stop
#define PLAY_STATE_STOP 1
// Currently playing
#define PLAY_STATE_PLAYING 2
// Currently playing but paused
#define PLAY_STATE_PAUSED 3
// Playing but finished
#define PLAY_STATE_FINISHED 4

char current_play_state = PLAY_STATE_NONE;

// quick hack - for now, manually need to load this at 0x3fd; I'll look
// into a c2d style loader wrapper later

// play a tape data block
void
play_tape_block(unsigned int len)
{
  // Consume the rest of the header - periods, pre-blank, post-blank
  unsigned char buf[3];
  unsigned char i, j = 0;
  unsigned char checksum = 0xff;
  unsigned int l;


  if (file_read_bytes(buf, 3) != 3) {
    Serial.println("ERR: not enough data in tape header");
    return;
  }

  // Setup for this block
  new_cassette_data_init();
  new_cassette_period_length_set(buf[0]);
  new_cassette_period_set_pre_blank(buf[1]);
  new_cassette_period_set_post_blank(buf[2]);

  // Rest of the length is the tape data itself
  len -= 3;

  new_cassette_data_set_length(len + 1L); // Include the checksum byte!

  // Start the cassette playback with the above info
  cassette_new_start();

  Serial.print("Writing ");
  Serial.print(len);
  Serial.print(" bytes:");

  // Start immediately filling the FIFO with data!
  for (l = 0; l < len; l++) {
    if (file_read_bytes(buf, 1) != 1) {
      Serial.println("ERR: ran out of data bytes");
      break;
    }
    //Serial.print(buf[0] & 0xff, HEX);
    while (new_cassette_data_add_byte(buf[0]) != true) {
      delay(1);
    }
    checksum ^= buf[0];
    
    if (j % 64 == 0) {
          Serial.print(".");
    }
    j++;
  }

  // Send "checksum" byte
  while (new_cassette_data_add_byte(checksum) != true) {
    delay(1);
  }
  Serial.println("\nTAPE: play block done.");
}

// Play a tape
// This blocks and will need to check for the UI in its main loop
// It also should only be called once the file is open.
//
void
play_tape(void)
{
  unsigned char buf[2];
  unsigned char hdr, h_type;
  unsigned int len;
  bool run_loop = true;

  while (run_loop == true) {
    // read header and type and length
    if (file_read_bytes(buf, 2) != 2) {
      Serial.println("ERR: out of hdr bytes");
      break;
    }
    hdr = buf[0]; h_type = buf[1];
    if (hdr != 0xef) {
      Serial.println("ERR: invalid header byte");
      break;
    }

    if (file_read_bytes(buf, 2) != 2) {
      Serial.println("ERR: out of len bytes");
      break;
    }
    len = ((int) buf[1]) << 8 | buf[0];

    switch (h_type) {
      case 0x01: // Pause
        Serial.println("TODO: implement pause here!");
        continue;
      case 0x02: // Data
        play_tape_block(len);
        continue;
      case 0x03: // Done
        Serial.println("STATE: tape is done.");
        run_loop = false;
        break;
      default:
        // Read file bytes for length, then next
        for (; len >= 0; len--) {
          if (file_read_bytes(buf, 1) != 1) {
            Serial.println("ERR: out of field bytes");
            run_loop = false;
          }
        }
    }
  }
  Serial.println("INFO: play_tape done.");  
}

void setup() {
  long file_size;
  
  Serial.begin(9600);
  display_setup();
  cassette_new_init();
  buttons_setup();
  display_clear();
  file_setup();


  // For now, pre-load a single file - apple_invaders.bin
  // to load/run in monitor - 3FD.537CR 3FDG
  // the data payload is 20352 bytes, the rest are the header bits
  
  file_open("apple_invaders.bin");
  
  play_tape();

  Serial.println("\n--\nDone.");

  // Now wait until the state is NONE
  while (cassette_new_get_state() != 0) {
    delay(1);
  }
  Serial.println("--\nFinished sending.");
  file_close();
}


void loop() {
}
