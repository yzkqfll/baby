
#include "Comdef.h"
#include "OSAL.h"
#include "hal_board.h"

#include "ther_uart.h"
#include "ther_uart_comm.h"

#include "ther_spi.h"
#include "ther_spi_w25x40cl.h"

#define MODULE "[W25X] "

void ther_spi_w25x_init(void)
{
	ther_spi_init();
}
