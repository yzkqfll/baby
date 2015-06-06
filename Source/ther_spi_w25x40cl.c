/*
 * spi flash w25x device driver
 *
 * Change Log:
 * =======================
 * 6Jun15, yue.hu, created for w25x40cl
 */

#include "comdef.h"
#include "OSAL.h"
#include "hal_board.h"
#include "ther_uart.h"
#include "ther_uart_comm.h"
#include "ther_spi.h"
#include "ther_spi_w25x40cl.h"

#define MODULE  "[W25X] "

#define CONFIG_W25X40CL

#ifdef CONFIG_W25X40CL
#define CHIP_SIZE           (512*1024) //512KB
#define PAGE_SIZE           (256)      //256B

#define SECTOR_COUNT        (128)
#define BYTES_PER_SECTOR    (4096)     //4KB

#define BLOCK1_COUNT        (16)
#define BLOCK1_SIZE         (32*1024) //32KB
#define BLOCK2_COUNT        (8)
#define BLOCK2_SIZE         (64*1024) //64KB
#endif

/* JEDEC Manufacturers ID */
#define MF_ID           (0xEF)
#define GD_ID           (0xC8)

/* JEDEC Device ID: Memory type and Capacity */
#define MTC_W25X40CL    (0x3013)

/* command list */
#define CMD_WRSR                    (0x01)  /* Write Status Register */
#define CMD_PP                      (0x02)  /* Page Program */
#define CMD_READ                    (0x03)  /* Read Data */
#define CMD_WRDI                    (0x04)  /* Write Disable */
#define CMD_RDSR                    (0x05)  /* Read Status Register */
#define CMD_WREN                    (0x06)  /* Write Enable */
#define CMD_FAST_READ               (0x0B)  /* Fast Read */
#define CMD_ERASE_4K                (0x20)  /* Sector Erase:4K */
#define CMD_ERASE_32K               (0x52)  /* 32KB Block Erase */
#define CMD_ERASE_64K               (0xD8)  /* 64KB Block Erase */
#define CMD_JEDEC_ID                (0x9F)  /* Read JEDEC ID */
#define CMD_ERASE_CHIP              (0xC7)  /* Chip Erase */
#define CMD_RELEASE_PWRDN           (0xAB)  /* Release device from power down state */

#define DUMMY                       (0xFF)

struct flash_device flash_dev;

static void w25x_enable(void)
{
	ther_spi_enable();
}

static void w25x_disable(void)
{
	ther_spi_disable();
}

static uint8 w25x_read_status(void)
{
	uint8 cmd = CMD_RDSR;
	uint8 value = 0;

	ther_spi_send(&cmd, 1);
	ther_spi_recv(&value, 1);

	return value;
}

static void w25x_wait_busy(void)
{
	while( w25x_read_status() & (0x01));
}

/** \brief read [size] byte from [offset] to [buffer]
 *
 * \param offset uint32 unit : byte
 * \param buffer uint8*
 * \param size uint32   unit : byte
 * \return uint32 byte for read
 *
 */
static uint32 w25x_read(uint32 offset, uint8 *buffer, uint32 size)
{
	uint8 send_buffer[4];

	send_buffer[0] = CMD_WRDI;
	ther_spi_send(send_buffer, 1);

	send_buffer[0] = CMD_READ;
	send_buffer[1] = (uint8)(offset >> 16);
	send_buffer[2] = (uint8)(offset >> 8);
	send_buffer[3] = (uint8)(offset);

	ther_spi_send(send_buffer, 4);
	ther_spi_recv(buffer, size);

	return size;
}

/** \brief write N page on [page]
 *
 * \param page_addr uint32 unit : byte (4096 * N,1 page = 4096byte)
 * \param buffer const uint8*
 * \return uint32
 *
 */
static uint32 w25x_page_write(uint32 page_addr, const uint8 *buffer)
{
	uint32 index;
	uint8 send_buffer[4];

	if ((page_addr & 0xFF) != 0) {
		print(LOG_ERR, MODULE "page addr must align to 256bytes,dead here!\r\n");
		while(1);
	}

	send_buffer[0] = CMD_WREN;
	ther_spi_send(send_buffer, 1);

	send_buffer[0] = CMD_ERASE_4K;
	send_buffer[1] = (page_addr >> 16);
	send_buffer[2] = (page_addr >> 8);
	send_buffer[3] = (page_addr);
	ther_spi_send(send_buffer, 4);

	w25x_wait_busy(); // wait erase done.

	for(index = 0; index < (PAGE_SIZE / 256); index++) {
		send_buffer[0] = CMD_WREN;
		ther_spi_send(send_buffer, 1);

		send_buffer[0] = CMD_PP;
		send_buffer[1] = (uint8)(page_addr >> 16);
		send_buffer[2] = (uint8)(page_addr >> 8);
		send_buffer[3] = (uint8)(page_addr);

		ther_spi_send(send_buffer, 4);
		ther_spi_send(buffer, 256);

		buffer += 256;
		page_addr += 256;
		w25x_wait_busy();
	}

	send_buffer[0] = CMD_WRDI;
	ther_spi_send(send_buffer, 1);

	return PAGE_SIZE;
}

static uint8 w25x_flash_init(void)
{
	return FL_EOK;
}

static uint8 w25x_flash_open(void)
{
	uint8 send_buffer[3];

	w25x_enable();

	send_buffer[0] = CMD_WREN;
	ther_spi_send(send_buffer, 1);

	send_buffer[0] = CMD_WRSR;
	send_buffer[1] = 0;
	send_buffer[2] = 0;
	ther_spi_send(send_buffer, 3);

	w25x_wait_busy();

	w25x_disable();

	return FL_EOK;
}

static uint8 w25x_flash_close(void)
{
	return FL_EOK;
}

static uint32 w25x_flash_read(int32 pos, void* buffer, uint32 size)
{
	w25x_enable();

	w25x_read(pos * BYTES_PER_SECTOR, buffer, size * BYTES_PER_SECTOR);

	w25x_disable();

	return size;
}

static uint32 w25x_flash_write(int32 pos, const void* buffer, uint32 size)
{
	uint32 i = 0;
	uint32 block = size;
	const uint8 *ptr = buffer;

	w25x_enable();

	while(block--) {
		w25x_page_write((pos + i)* BYTES_PER_SECTOR, ptr);
		ptr += PAGE_SIZE;
		i++;
	}

	w25x_disable();

	return size;
}

uint8 ther_spi_w25x_init(void)
{
	struct flash_device *fd = &flash_dev;
	uint8 cmd;
	uint8 id_recv[3] = {0, 0, 0};
	uint16 memory_type_capacity;

	/* init spi */
	ther_spi_init();

	/* read flash id */
	ther_spi_enable();

	cmd = CMD_JEDEC_ID;
	ther_spi_send(&cmd, 1);
	ther_spi_recv(id_recv, 3);

	ther_spi_disable();

	if(id_recv[0] != MF_ID) {
		print(LOG_INFO, MODULE "Manufacturers ID(%x) error!\r\n", id_recv[0]);
		return FL_EID;
	}

	/* get the geometry information */
	fd->sector_count     = SECTOR_COUNT;
	fd->bytes_per_sector = BYTES_PER_SECTOR;
	fd->page_size        = PAGE_SIZE;

	/* get memory type and capacity */
	memory_type_capacity = id_recv[1];
	memory_type_capacity = (memory_type_capacity << 8) | id_recv[2];

	if(memory_type_capacity == MTC_W25X40CL) {
		print(LOG_INFO, MODULE "W25X40CL detection is ok\r\n");
	} else {
		print(LOG_INFO, MODULE "memory type(%x) capacity(%x) error!\r\n", id_recv[1], id_recv[2]);
		return FL_ETYPE;
	}

	/* callback */
	fd->init    = w25x_flash_init;
	fd->open    = w25x_flash_open;
	fd->close   = w25x_flash_close;
	fd->read    = w25x_flash_read;
	fd->write   = w25x_flash_write;

	return FL_EOK;
}

