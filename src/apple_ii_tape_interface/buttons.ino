#include "config.h"
#include "buttons.h"

void
buttons_setup(void)
{
  /* Configure buttons as pull-up, inputs */
  pinMode(PLAY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PREV_BUTTON_PIN, INPUT_PULLUP);
  pinMode(NEXT_BUTTON_PIN, INPUT_PULLUP);
}

char
buttons_read(void)
{
  char ret = 0;
  
  if (digitalRead(PLAY_BUTTON_PIN) == LOW) {
    ret |= BUTTON_FIELD_PLAY;
  }

  if (digitalRead(STOP_BUTTON_PIN) == LOW) {
    ret |= BUTTON_FIELD_STOP;
  }

  if (digitalRead(PREV_BUTTON_PIN) == LOW) {
    ret |= BUTTON_FIELD_PREV;
  }

  if (digitalRead(NEXT_BUTTON_PIN) == LOW) {
    ret |= BUTTON_FIELD_NEXT;
  }
  
  return ret;
}
