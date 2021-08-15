#include <TimerOne.h>

// tracking whether the pin is high or low, may end up using it for other things
char pin_state = 0;

#define CURRENT_MODE_NONE 0
#define CURRENT_MODE_PRE_BLANK 1
#define CURRENT_MODE_HEADER 2
#define CURRENT_MODE_SYNC 3
#define CURRENT_MODE_DATA 4
#define CURRENT_MODE_DATA_FINISH 5
#define CURRENT_MODE_POST_BLANK 6

volatile char current_mode = CURRENT_MODE_NONE;
int current_mode_arg = 0;

// Configuration parameters for the current record send
unsigned char config_period = 0; // in 128 period blocks
unsigned char config_pre_blank_period = 0; // in 100mS blocks
unsigned char config_post_blank_period = 0; // in 100mS blocks

#define BUFFER_SIZE 64

// Data writing state
unsigned char current_byte; // Which byte we're writing
char current_bit_pos; // which bit in the byte we're writing out

/* buffer stuff - modified in both the ISR and main contexts */
volatile unsigned char send_buffer[BUFFER_SIZE];
volatile unsigned int total_write_length = 0; // How much data to write in total (incl checksum, that's calculated and sent by the producer layer)
volatile char buffer_head = 0;
volatile char buffer_tail = 0;

void
buf_init(void)
{
  buffer_head = buffer_tail = 0;
}

bool
buf_add_data(unsigned char val)
{
  char n;
  n = buffer_head + 1;
  if (n >= BUFFER_SIZE) {
    n = 0;
  }
  
  if (n == buffer_tail) {
    return false;
  }
  send_buffer[buffer_head] = val;
  buffer_head = n;
  return true;
}

bool
buf_read_data(unsigned char *val)
{
  char n;
  if (buffer_head == buffer_tail) {
    return false;
  }
  n = buffer_tail + 1;
  if (n >= BUFFER_SIZE) {
    n = 0;
  }
  *val = send_buffer[buffer_tail];
  buffer_tail = n;
  return true;
}

/*
 * Initialise the buffer sending machinery.
 * This must be called before data is queued into the sending
 * path.
 */
void
new_cassette_data_init(void)
{
  noInterrupts();
  buf_init();
  total_write_length = 0;
  config_period = 0;
  config_pre_blank_period = 0;
  config_post_blank_period = 0;
  interrupts();
}

/*
 * Set the length of data being written out in bytes.
 * 
 * This includes the checksum, as the checksum will be provided
 * by the upper layer (which simplifies the data flow states at
 * this layer.)
 */
void
new_cassette_data_set_length(unsigned int val)
{
  total_write_length = val;
}

/*
 * Set the header period length.
 */
void
new_cassette_period_length_set(char val)
{
  config_period = val;
}

/*
 * Set the wait time before sending the header, in 100mS increments.
 */
void
new_cassette_period_set_pre_blank(char val)
{
  config_pre_blank_period = val;
}

/*
 * Set the wait time after sending the header, in 100mS increments.
 */
void
new_cassette_period_set_post_blank(char val)
{
  config_post_blank_period = val;
}

/*
 * Push data into the outbound queue.
 * 
 * This will disable interrupts and push a byte into the queue.
 * It will return true if the byte was pushed, and false
 * if the queue is full.
 */
bool
new_cassette_data_add_byte(unsigned char val)
{
  bool r;

  noInterrupts();
  r = buf_add_data(val);
  interrupts();
  return r;
}

void
pin_set(char val)
{
  digitalWrite(SPEAKER_PIN, val == 0 ? LOW : HIGH);
  pin_state = val;
}

void
pin_flip(void)
{
  pin_set(! pin_state);
}

// ISR for timer1 for pin flipping
void
cassette_new_isr(void)
{
  unsigned long fudgeTime = micros();         //fudgeTime is used to reduce length of the next period by the time taken to process the ISR

  long next_timer_interval = 0;

  noInterrupts();

  if (current_mode == CURRENT_MODE_NONE) {
      Timer1.stop();
      interrupts();
      return;
  }

  /*
   * If we're sending a header, we need to flip the pin
   * high and low every period, decrement period, and then
   * when we hit the last period flip the state to send
   * the sync bit.
   */
  if (current_mode == CURRENT_MODE_HEADER) {
    pin_flip();
    next_timer_interval = 650;
    if (pin_state == 0) {
      current_mode_arg--; // Decrement period only on low transition
    }

    if (current_mode_arg <= 0) {
      // Finished, bump to sync for next timer
      current_mode = CURRENT_MODE_SYNC;
      current_mode_arg = 0;
    }
    goto done;
  }

  /*
   * SYNC - we're doing a low -> high -> low transition, and we
   * assume we start low.
   */
  if (current_mode == CURRENT_MODE_SYNC) {
    pin_flip();
    if (pin_state == 1) {
      next_timer_interval = 200;
    } else {
      next_timer_interval = 250;
      // Next transition is to start data playback
      current_mode = CURRENT_MODE_DATA;
      current_mode_arg = 0;
      current_bit_pos = 7;
      // XXX TODO: handle error here!
      (void) buf_read_data(&current_byte);
    }
    goto done;
  }

  /*
   * Data finish state
   */
  if (current_mode == CURRENT_MODE_DATA_FINISH) {
    pin_flip();
    if (pin_state == 1) {
      next_timer_interval = 10000; // 10mS
    } else {
      // Next state is post blank
      pin_set(0);
      current_mode = CURRENT_MODE_POST_BLANK;
      current_mode_arg = config_post_blank_period;
      next_timer_interval = 100L*1000L; // 100mS
    }
    goto done;
  }

  /*
   * Pre-blank - loop over, ensure the state stays low.
   * current_arg is configured with the number of 
   */
  if (current_mode == CURRENT_MODE_PRE_BLANK) {
    pin_set(0);
    if (current_mode_arg == 0) {
      current_mode = CURRENT_MODE_HEADER;
      current_mode_arg = config_period;
      next_timer_interval = 650;
    } else {
      current_mode_arg--;
      next_timer_interval = 100L * 1000L;
    }
    goto done;
  }

  /*
   * Post-blank - loop over, ensure the state stays low.
   * current_arg is configured with the number of 
   */
  if (current_mode == CURRENT_MODE_POST_BLANK) {
    pin_set(0);
    if (current_mode_arg == 0) {
      current_mode = CURRENT_MODE_NONE;
      next_timer_interval = 0;
    } else {
      current_mode_arg--;
      next_timer_interval = 100L * 1000L;
    }
    goto done;
  }

  /*
   * The fun one - data.  This is going to be the super
   * fun challenge!
   */
  if (current_mode == CURRENT_MODE_DATA) {
    /*
     * 1 is 1000Hz (500uS), 0 is 2000Hz (250uS).
     * So we flip the pin here, and sleep for the current bit.
     * If the transition is to a low state, that's a sign we need
     * to shift our bit position, and when we run out of
     * bits to shift we consume another byte.
     */
     pin_flip();
     if (current_byte & (1 << current_bit_pos)) {
       next_timer_interval = 500;
     } else {
       next_timer_interval = 250;
     }

     if (pin_state == 0) {
      // Next bit
      current_bit_pos--;
      if (current_bit_pos == 0) {
        total_write_length--;

        // Check if we have any further data left that we need to consume
        if (total_write_length == 0) {
          current_mode = CURRENT_MODE_DATA_FINISH;
          goto done;
        }
        
        // Next byte - try to consume it!
        current_bit_pos = 7;
        if (buf_read_data(&current_byte) == false) {
          // XXX TODO: handle underflow! Signal error somehow!
          next_timer_interval = 0;
          current_mode = CURRENT_MODE_NONE;
        }
      }
     }
     goto done;
  }

  // No work done here - disable timer
  next_timer_interval = 0;
  
done:
  if (next_timer_interval == 0) {
    Timer1.stop();
    pin_set(0);
  } else {
    fudgeTime = micros() - fudgeTime; //Compensate for stupidly long ISR
    if (next_timer_interval < fudgeTime) {
      // XXX We messed up, need to flag this
      Timer1.setPeriod(1);
    } else {
      Timer1.setPeriod(next_timer_interval - fudgeTime);
    }
  }
  interrupts();
}

void
cassette_new_init(void)
{
  Timer1.initialize(100000);
  Timer1.attachInterrupt(cassette_new_isr);
  Timer1.stop();
}

/*
 * Start sending a header+data block.
 */
void
cassette_new_start(void)
{
  current_mode = CURRENT_MODE_PRE_BLANK;
  current_mode_arg  = config_pre_blank_period;
  pin_set(0);
  Timer1.setPeriod(100);
  Timer1.start();
}
