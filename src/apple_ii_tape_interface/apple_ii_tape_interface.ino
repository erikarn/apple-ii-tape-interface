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

int current_ui_file = -1;
int current_ui_filecount = -1;
char current_ui_filename[20];

// Used to determine whether to check for a button press
unsigned long button_millis = 0;


// quick hack - for now, manually need to load this at 0x3fd; I'll look
// into a c2d style loader wrapper later

bool
play_tape_check_stop_button(void)
{
  if (millis() - button_millis < 50) {
    return false;
  }
  button_millis = millis();
    
  char btn = buttons_read();
  if (btn & BUTTON_FIELD_STOP) {
    return true;
  }
  return false;
}

/*
 * play a tape data block
 * Returns true if completed, false if there was an error and we should
 * abort the rest of the playback.
 */
bool
play_tape_block(unsigned int len)
{
  // Consume the rest of the header - periods, pre-blank, post-blank
  unsigned char buf[3];
  unsigned char i, j = 0;
  unsigned char checksum = 0xff;
  unsigned int l;


  if (file_read_bytes(buf, 3) != 3) {
    Serial.println("ERR: not enough data in tape header");
    return false;
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
    // XXX Check for stop button - total hack for now!
    if (play_tape_check_stop_button() == true) {
      cassette_new_force_stop();
      return false;
    }

    if (file_read_bytes(buf, 1) != 1) {
      Serial.println("ERR: ran out of data bytes");
      return false;
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
  return true;
}

/*
 * Play a tape
 * This blocks and will need to check for the UI in its main loop
 * It also should only be called once the file is open.
 * 
 * Returns true if the tape was played correctly, false if
 * we hit an error.
 */
bool
play_tape(void)
{
  unsigned char buf[2];
  unsigned char hdr, h_type;
  unsigned int len;
  bool run_loop = true;
  bool retval = true;

  /*
   * Seek to the beginning; if we fail then treat it as the file
   * being unselected or unavailable.
   */
  if (file_seek_beginning() == false) {
    retval = false;
    goto done;
  }

  retval = true;
  while (run_loop == true) {
    // XXX - Check for stop button; total hack!
    if (play_tape_check_stop_button() == true) {
      cassette_new_force_stop();
      retval = true; // Stopping isn't an error here
      break;
    }
    
    // read header and type and length
    if (file_read_bytes(buf, 2) != 2) {
      Serial.println("ERR: out of hdr bytes");
      retval = false;
      break;
    }
    hdr = buf[0]; h_type = buf[1];
    if (hdr != 0xef) {
      Serial.println("ERR: invalid header byte");
      retval = false;
      break;
    }

    if (file_read_bytes(buf, 2) != 2) {
      Serial.println("ERR: out of len bytes");
      retval = false;
      break;
    }
    len = ((int) buf[1]) << 8 | buf[0];

    switch (h_type) {
      case 0x01: // Pause
        Serial.println("TODO: implement pause here!");
        continue;
      case 0x02: // Data
        if (play_tape_block(len) == false) {
          // XXX would be nice to know if we were stopped, or an error
          run_loop = false;
          retval = false;
          break;
        }
        continue;
      case 0x03: // Done
        Serial.println("STATE: tape is done.");
        run_loop = false;
        retval = true;
        break;
      default:
        // Read file bytes for length, then next
        for (; len >= 0; len--) {
          if (file_read_bytes(buf, 1) != 1) {
            Serial.println("ERR: out of field bytes");
            run_loop = false;
            retval = false;
            break;
          }
          break;
        }
    }
  }
done:
  Serial.print("INFO: play_tape done: ");
  if (retval) {
    Serial.println("OK.");
  } else { 
    Serial.println("Error.");
  }
  return retval;
}

void
ui_init_first_file(void)
{
  if (current_ui_file != -1) {
    return;
  }
  int fc = dir_get_file_count();
  if (fc < 0) {
    return;
  }
  current_ui_filecount = fc;
  if (current_ui_filecount == 0) {
    // No files that can be displayed
    return;
  }

  // Read the first filename
  fc = dir_get_file_by_index(0, current_ui_filename, 20);
  if (fc <= 0) {
    return;
  }
  current_ui_file = 0;

  // Ok, we're ready - we have a file count and filename!
}

void
ui_display_current_file(void)
{
  if (current_ui_file == -1) {
    Serial.println("Current file: <unavailable>");
    return;
  }
  Serial.print("Current file: ");
  Serial.println(current_ui_filename);
}

void setup() {
  long file_size;
  
  Serial.begin(9600);
  display_setup();
  cassette_new_init();
  buttons_setup();
  display_clear();
  file_setup();
  ui_init_first_file();

  ui_display_current_file();

#if 0
  // For now, pre-load a single file - apple_invaders.bin
  // to load/run in monitor - 3FD.537CR 3FDG
  // the data payload is 20352 bytes, the rest are the header bits
  
  file_open("apple_panic.bin");
  
  play_tape();

  Serial.println("\n--\nDone.");

  // Now wait until the state is NONE
  while (cassette_new_get_state() != 0) {
    delay(1);
  }
  Serial.println("--\nFinished sending.");
  file_close();
#endif
}


void
ui_loop(void)
{
    if (millis() - button_millis < 50) {
      return;
    }
    button_millis = millis();
    
  char btn = buttons_read();

  if (btn & BUTTON_FIELD_PREV) {
    if ((current_ui_file != -1) && (current_ui_filecount != -1) && (current_ui_file > 0)) {
      current_ui_file--;
      if (dir_get_file_by_index(current_ui_file, current_ui_filename, 20) != 1) {
        // XXX ew, need a proper "this index is busted"
        current_ui_filename[0] = '*';
        current_ui_filename[1] = '!';
        current_ui_filename[2] = '\0';
      }
      ui_display_current_file();
    }

    while (buttons_read() & BUTTON_FIELD_PREV) {
      delay(50);
    }
  }
  if (btn & BUTTON_FIELD_NEXT) {
    if ((current_ui_file != -1) && (current_ui_filecount != -1) && (current_ui_file < current_ui_filecount - 1)) {
      current_ui_file++;
      if (dir_get_file_by_index(current_ui_file, current_ui_filename, 20) != 1) {
        // XXX ew, need a proper "this index is busted"
        current_ui_filename[0] = '*';
        current_ui_filename[1] = '!';
      }
      ui_display_current_file();
    }

    while (buttons_read() & BUTTON_FIELD_NEXT) {
      delay(50);
    }
  }

  if (btn & BUTTON_FIELD_PLAY) {
    Serial.println("PLAY");
     while (buttons_read() & BUTTON_FIELD_PLAY) {
      delay(50);
    }

    // For now this is blocking, it'll have to poll STOP itself
    play_tape();
  }

  
  if (btn & BUTTON_FIELD_STOP) {
    Serial.println("STOP");
     while (buttons_read() & BUTTON_FIELD_STOP) {
      delay(50);
    }
  }
  
}

void loop() {

  ui_loop();
}
