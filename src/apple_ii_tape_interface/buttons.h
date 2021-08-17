#ifndef __APPLE_BUTTONS_H__
#define __APPLE_BUTTONS_H__

#define BUTTON_FIELD_PLAY 0x1
#define BUTTON_FIELD_STOP 0x2
#define BUTTON_FIELD_PREV 0x4
#define BUTTON_FIELD_NEXT 0x8

extern void buttons_setup(void);
extern char buttons_read(void);

#endif /* __APPLE_BUTTONS_H__ */
