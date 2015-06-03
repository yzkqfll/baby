
#ifndef __THER_OLED_9639_H__
#define __THER_OLED_9639_H__

enum {
	VCC_POWER_OFF = 0,
	VCC_POWER_ON
};

enum {
	VDD_POWER_OFF = 0,
	VDD_POWER_ON,
};

enum {
	DISPLAY_OFF = 0,
	DISPLAY_ON
};

void oled_init(void);
void oled_display_on(void);
void oled_light_off(void);
void oled_show_welcome(void);
void oled_picture_inverse(void);

#endif

