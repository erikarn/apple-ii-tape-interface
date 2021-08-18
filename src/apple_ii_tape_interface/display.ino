
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void
display_setup(void)
{
  lcd.init();
  lcd.backlight();
}

void
display_clear(void)
{
  lcd.clear();
}

void
display_line1(const char *f)
{
  lcd.setCursor(0, 0);
  lcd.print(f);
}

// XXX de-dup
void
display_line1_f(const __FlashStringHelper *c)
{
  char j;
  PGM_P p = reinterpret_cast<PGM_P>(c);

  lcd.setCursor(0, 0);
  while ((j = pgm_read_byte(p++)) != 0) {
    lcd.print(j);
  }
}

void
display_line2(const char *f)
{
  lcd.setCursor(0, 1);
  lcd.print(f);
}

void
display_line2_at(char pos, const char *f)
{
  lcd.setCursor(pos, 1);
  lcd.print(f);
}

void
display_line2_f(const __FlashStringHelper *c)
{
  char j;
  PGM_P p = reinterpret_cast<PGM_P>(c);

  lcd.setCursor(0, 1);
  while ((j = pgm_read_byte(p++)) != 0) {
    lcd.print(j);
  }
}


void
display_line2_blank(void)
{
    display_line2_f(F("                "));
}
