
#ifndef __THER_SPI_H__
#define __THER_SPI_H__

/**
 * SPI message structure
 */
struct ther_spi_message {
	const void *send_buf;
	void *recv_buf;
	uint32 length;
};

/**
 * SPI common interface
 */
void ther_spi_init(void);
uint32 ther_spi_recv(void *recv_buf, uint32 length);
uint32 ther_spi_send(const void *send_buf, uint32 length);
void ther_spi_enable(void);
void ther_spi_disable(void);

#endif

