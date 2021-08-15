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
