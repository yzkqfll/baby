
#include "Comdef.h"
#include "OSAL.h"

#include "uart.h"
#include "uart_comm.h"

#include "oled_9639.h"

#define MODULE "[OLED9639] "

#define OLED_IIC_ADDR 0x78

#define BUF_LEN 2

#define TYPE_CMD 0x0
#define TYPE_DATA 0x40

#define MAX_COL 96
#define MAX_ROW 39

#define MAX_PAGE 5 /* 8, 8, 8, 8, 7 = 39 lines */

/*
 * 1. Fundamental Command Table
 */
#define CMD_CONTRAST 0x81

enum {
	ENTIRE_ON = 0,
	ENTIRE_OFF,
};
#define CMD_ENTIRE_DISPLAY(x) (0xA4 + (x))

enum {
	INVERSE_OFF = 0, /* normal display */
	INVERSE_ON,
}
#define CMD_DISPLAY_INVERSE(x) (0xA6 + (x))

#define CMD_DISPLAY_OFF 0xAE
#define CMD_DISPALY_ON 0xAF



/*
 * 2. Scrolling Command Table
 */

/*
 * 3. Addressing Setting Command Table
 */
#define CMD_START_COL_LOW(x) (0x0 + (x) % 16)
#define CMD_START_COL_HIGH(x) (0x10 + (x) / 16)

#define CMD_START_PAGE(x) (0xB0 + (x))

/*
 * 4. Hardware Configuration (Panel resolution & layout related) Command Table
 */
#define CMD_DISP_START_LINE(x) (0x40 + (x)) /* x: [0, 63]*/
#define CMD_MULTIPLEX_RATIO 0xA8

enum {
	REMAP_OFF = 0,
	REMAP_ON,
};
#define CMD_SEGMENT_REMAP(x) (0xA0 + (x))

/*
 * 5. Timing & Driving Scheme Setting Command Table
 */
#define CMD_DISP_CLK_DIV 0xD5

#define CMD_PRECHARGE_PERIOD 0xD9

enum {
	VCOMH_LEVEL_LOW = 0x00, /* ~ 0.65 x VCC */
	VCOMH_LEVEL_MID = 0x20, /* ~ 0.77 x VCC (RESET) */
	VCOMH_LEVEL_HIGH = 0x30, /* ~ 0.83 x VCC */
};
#define CMD_VCOMH_DESELECT 0xDB

/*
 * Command Table for Charge Bump Setting
 */
#define CMD_CHARGE_PUMP 0x8D
	#define SET_CHARGE_PUMP(x) ((x) << 2)

/*
 * Send command to OLED
 * slave addr + type + cmd
 */
static void send_cmd(unsigned char cmd)
{
	unsigned char buf[BUF_LEN] = {TYPE_CMD, cmd};

	HalI2CWrite(BUF_LEN, buf);
}

/*
 * Send data to OLED
 * slave addr + type + data
 */
static void send_data(unsigned char data)
{
	unsigned char buf[BUF_LEN] = {TYPE_DATA, data};

	HalI2CWrite(BUF_LEN, buf);
}

/*
 * 1. Fundamental Command
 */


static void set_contrast(unsigned char steps)
{
	send_cmd(CMD_CONTRAST);

	/* 0 ~ 0x7f */
	send_cmd(steps);
}

static void set_entire_display_on(unsigned char val)
{
	send_cmd(CMD_ENTIRE_DISPLAY(val));
}

static void set_display(unsigned char val)
{
	send_cmd(CMD_DISPLAY_INVERSE(val));
}

static void set_display_on(void)
{
	send_cmd(CMD_DISPALY_ON);
}

static void set_display_off(void)
{
	send_cmd(CMD_DISPLAY_OFF);
}

/*
 * 2. Scrolling Command
 */


/*
 * 3. Addressing Setting Command
 */


/*
 * Set Page Start Address for Page Addressing Mode
 */
static void set_start_page(unsigned char start_page)
{
	/*
	 * OLED 96x39 has 39 lines, 8 line per page.
	 * 39 lines => 5 page
	 *
	 * valid range [0, 4]
	 */

	send_cmd(CMD_START_PAGE(start_page));
}

static void set_start_column(unsigned char start_column)
{
	/*
	 * 96 columns
	 */

	send_cmd(CMD_START_COL_LOW(start_column));
	send_cmd(CMD_START_COL_HIGH(start_column));
}

/*
 * 4. Hardware Configuration (Panel resolution & layout related) Command
 */

static void set_start_line(unsigned char line)
{
	send_cmd(CMD_DISP_START_LINE(line));
}

static void set_segment_remap(unsigned char val)
{
	send_cmd(CMD_SEGMENT_REMAP(val));
}

static void set_multiplex_ratio(unsigned char ratio)
{
	send_cmd(CMD_MULTIPLEX_RATIO);
	send_cmd(ratio);
}

/*
 * 5. Timing & Driving Scheme Setting Command
 */


static void set_display_clock(unsigned char clk_div)
{
	send_cmd(CMD_DISP_CLK_DIV);

	/*
	 * D[3:0] => Display Clock Divider
	 * D[7:4] => Oscillator Frequency
	 */
	send_cmd(clk_div);
}

static void set_precharge_period(unsigned char val)
{
	send_cmd(CMD_PRECHARGE_PERIOD);

	/*
	 * val[3:0] : Phase 1 period of up to 15 DCLK
	 *
	 * val[7:4] : Phase 2 period of up to 15 DCLK
	 */
	send_cmd(val);
}

static void set_vcomh_deselect(unsigned val)
{
	send_cmd(CMD_VCOMH_DESELECT);

	/* A[6:4] */
	send_cmd(val);
}

/*
 * MISC
 */
static void set_charge_pump(unsigned char val)
{
	send_cmd(CMD_CHARGE_PUMP);
	send_cmd(SET_CHARGE_PUMP(val));
}

static void fill_block(unsigned char start_page, unsigned char end_page,
		unsigned char start_col, unsigned char end_col, unsigned char *data)
{
	unsigned char page;
	unsigned char col;

	for (page = start_page; page <= end_page; page++) {

		set_start_page(page);

		for (col = start_col; col <= end_col; col++) {
			send_data(*data++);
		}
	}
}

static void fill_screen(unsigned char val)
{
//	fill_block(0, MAX_PAGE, 0, MAX_COL, val);
}

void oled_show_welcome(void)
{

//	fill_block(0, 0, 0, 4, );
}

void oled_show_temp(unsigned char temp)
{

}

void oled_show_batt(unsigned char level)
{

}

void oled_show_bt_link(bool linked)
{

}

void oled_init(void)
{
	print(LOG_INFO, MODULE "oled9639 init ok\r\n");

	HalI2CInit(OLED_IIC_ADDR, i2cClock_33KHZ);

	return;
}
