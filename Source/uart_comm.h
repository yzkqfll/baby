
#ifndef __UART_COMM_H__
#define __UART_COMM_H__

void uart_comm_init(void);

enum {
	LOG_DBG = 1,
	LOG_INFO,
	LOG_WRANING,
	LOG_ERR,
	LOG_CRIT
};

int print(unsigned char level, char *fmt, ...);

#endif

