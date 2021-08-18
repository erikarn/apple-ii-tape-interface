#include "config.h"

#include "buttons.h"

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

// Should unify this somehow to save space
bool
play_tape_check_play_button(void)
{
  if (millis() - button_millis < 50) {
    return false;
  }
  button_millis = millis();
    
  char btn = buttons_read();
  if (btn & BUTTON_FIELD_PLAY) {
    return true;
  }
  return false;
}

void
ui_display_current_file(void)
{
  display_clear();
  if (current_ui_file == -1) {
    display_line1("<unavail>");
    Serial.println(F("Current file: <unavailable>"));
    return;
  }
  Serial.print(F("Current file: "));
  Serial.println(current_ui_filename);

  display_line1(current_ui_filename);
}

void
ui_display_play_details(unsigned int start_addr, unsigned int end_addr)
{
  char buf[17] = { 0 };
  

  snprintf(buf, 16, "%X.%XR %XG", start_addr, end_addr, start_addr);
  Serial.print(F("Load: "));
  Serial.println(buf);
  display_line2(buf);
}

void
ui_display_play(void)
{
  Serial.println(F("PLAY"));
  display_line2_blank();
  display_line2("PLAY:");
}
/*
 * play a tape data block
 * Returns true if completed, false if there was an error and we should
 * abort the rest of the playback.
 */
bool
play_tape_block(unsigned int len, bool play_wait)
{
  // Consume the rest of the header - periods, pre-blank, post-blank
  unsigned char buf[8];
  unsigned char i, j = 0, k = 0;
  unsigned char checksum = 0xff;
  unsigned int l;
  unsigned int start_addr, end_addr;

  if (file_read_bytes(buf, 7) != 7) {
    Serial.println(F("ERR: not enough data in tape header"));
    display_line2_blank();
    display_line2("ERR: short tapehdr");
    return false;
  }

  // Setup for this block
  new_cassette_data_init();
  new_cassette_period_length_set(buf[0]);
  new_cassette_period_set_pre_blank(buf[1]);
  new_cassette_period_set_post_blank(buf[2]);

  start_addr = (unsigned int) buf[4] << 8 | buf[3];
  end_addr = (unsigned int) buf[6] << 8 | buf[5];

  ui_display_play_details(start_addr, end_addr);


  // Rest of the length is the tape data itself
  len -= 7;

  // Wait for play to be pressed again
  if (play_wait == true) {
    while (play_tape_check_play_button() == false)
      ;
  }
  new_cassette_data_set_length(len + 1L); // Include the checksum byte!

  ui_display_play();

  // Start the cassette playback with the above info
  cassette_new_start();

  Serial.print(F("Writing "));
  Serial.print(len);
  Serial.print(F(" bytes:"));

  // Start immediately filling the FIFO with data!
  for (l = 0; l < len; l++) {
    // XXX Check for stop button - total hack for now!
    if (play_tape_check_stop_button() == true) {
      cassette_new_force_stop();
      display_line2_blank();
      display_line2("NOTE: stopped");
      return false;
    }

    if (file_read_bytes(buf, 1) != 1) {
      Serial.println(F("ERR: ran out of data bytes"));
      display_line2_blank();
      display_line2("ERROR: short data");
      return false;
    }
    //Serial.print(buf[0] & 0xff, HEX);
    while (new_cassette_data_add_byte(buf[0]) != true) {
      delay(1);
    }
    checksum ^= buf[0];
    
    if (j % 64 == 0) {
          if (k == 0) {
            k = 1;
            Serial.print(F("+"));
            display_line2_at(6, "+");
          } else {
            k = 0;
            Serial.print(F("-"));
            display_line2_at(6, "-");
          }
       }
    j++;
  }

  // Send "checksum" byte
  while (new_cassette_data_add_byte(checksum) != true) {
    delay(1);
  }
  Serial.println(F("\nTAPE: play block done."));
  display_line2_blank();
  display_line2("DONE.");
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
  bool play_wait_flag = true;

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
      Serial.println(F("ERR: out of hdr bytes"));
      display_line2_blank();
      display_line2("ERR: hdr short");
      retval = false;
      break;
    }
    hdr = buf[0]; h_type = buf[1];
    if (hdr != 0xef) {
      Serial.println(F("ERR: invalid header byte"));
      display_line2_blank();
      display_line2("ERR: hdr invalid");
      retval = false;
      break;
    }

    if (file_read_bytes(buf, 2) != 2) {
      Serial.println(F("ERR: out of len bytes"));
      display_line2_blank();
      display_line2("ERR: len short");
      retval = false;
      break;
    }
    len = ((int) buf[1]) << 8 | buf[0];

    switch (h_type) {
      case 0x01: // Pause
        Serial.println(F("TODO: implement pause here!"));
        continue;
      case 0x02: // Data
        if (play_tape_block(len, play_wait_flag) == false) {
          // XXX would be nice to know if we were stopped, or an error
          run_loop = false;
          retval = false;
          break;
        }
        play_wait_flag = false; // Don't wait to press play for subsequent blocks, we need an explicit pause
        continue;
      case 0x03: // Done
        display_line2_blank();
        display_line2("DONE!");
        Serial.println(F("STATE: tape is done."));
        run_loop = false;
        retval = true;
        break;
      default:
        // Read file bytes for length, then next
        for (; len >= 0; len--) {
          if (file_read_bytes(buf, 1) != 1) {
            Serial.println(F("ERR: out of field bytes"));
            display_line2_blank();
            display_line2("ERR: field short");
            run_loop = false;
            retval = false;
            break;
          }
          break;
        }
    }
  }
done:
  Serial.print(F("INFO: play_tape done: "));

  if (retval) {
    Serial.println(F("OK."));
    display_line2_blank();
    display_line2("DONE!");

  } else { 
    Serial.println(F("Error."));
    display_line2_blank();
    display_line2("ERROR!");
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



void setup() {
  long file_size;
  
  Serial.begin(115200);
  display_setup();
  cassette_new_init();
  buttons_setup();
  display_clear();
  file_setup();
  ui_init_first_file();

  ui_display_current_file();
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
    Serial.println(F("PLAY"));
     while (buttons_read() & BUTTON_FIELD_PLAY) {
      delay(50);
    }

    // For now this is blocking, it'll have to poll STOP itself
    play_tape();
  }

  
  if (btn & BUTTON_FIELD_STOP) {
    Serial.println(F("STOP"));
     while (buttons_read() & BUTTON_FIELD_STOP) {
      delay(50);
    }
  }
  
}

void loop() {

  ui_loop();
}
