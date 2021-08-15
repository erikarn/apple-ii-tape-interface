#ifndef __APPLE_CASSETTE_H__
#define __APPLE_CASSETTE_H__

extern void cassette_header(unsigned short periods);
extern void cassette_write_byte(unsigned char val);
extern void cassette_write_block_progmem(const uint8_t *ptr, unsigned short len);
extern void cassette_setup(void);

#endif /* __APPLE_CASSETTE_H__ */
