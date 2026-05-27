#ifndef JW01_H
#define JW01_H

#include <stdint.h>

void jw01_init(void);
int  jw01_read(void);
uint16_t jw01_get_co2(void);

#endif
