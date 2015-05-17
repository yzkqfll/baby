
#ifndef __THER_BLE_H__
#define __THER_BLE_H__

unsigned char ther_ble_init(uint8 task_id);

void ble_start_advertise(void);
unsigned short ble_get_gap_handle(void);

#endif

