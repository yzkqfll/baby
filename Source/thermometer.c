
#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "OnBoard.h"
#include "hal_adc.h"
#include "hal_led.h"
#include "hal_lcd.h"
#include "hal_key.h"
#include "gatt.h"
#include "hci.h"
#include "gapgattserver.h"
#include "gattservapp.h"
#include "gatt_profile_uuid.h"
#include "linkdb.h"
#include "peripheral.h"
#include "gapbondmgr.h"
#include "ther_profile.h"
#include "devinfoservice.h"
#include "thermometer.h"
#include "timeapp.h"
#include "OSAL_Clock.h"

#include "uart.h"
#include "uart_comm.h"

#include "ther_ble.h"

#include "ther_comm.h"

#include "ther_button.h"

#define MODULE "[THER] "

struct ther_info {
	uint8 task_id;

	/*
	 * Indication
	 */
	bool temp_indication_enable;
	unsigned char indication_interval; /* second */

	/*
	 * Notification
	 */
	bool temp_notification_enable;
	unsigned char notification_interval; /* second */
};

static struct ther_info ther_info;

#define INTERVAL_MS(sec) ((sec) * 1000)

static void ther_temp_periodic_meas(struct ther_info *ti)
{
	/* need wait for response?? */
	ther_send_temp_indicate(ble_get_gap_handle(), ti->task_id);

	if (ti->temp_indication_enable)
		osal_start_timerEx( ti->task_id, TH_PERIODIC_MEAS_EVT, INTERVAL_MS(ti->indication_interval));

	return;
}

static void ther_temp_periodic_imeas(struct ther_info *ti)
{
	ther_send_temp_notify(ble_get_gap_handle());

	if (ti->temp_notification_enable)
		osal_start_timerEx( ti->task_id, TH_PERIODIC_IMEAS_EVT, INTERVAL_MS(ti->notification_interval));

	return;
}

static void ther_handle_gatt_access_msg(struct ther_info *ti, struct ble_msg *msg)
{
	switch (msg->type) {

	case GATT_TEMP_IND_ENABLED:
		print(LOG_INFO, MODULE "start temp indication\r\n");

		ti->temp_indication_enable = TRUE;
		ti->indication_interval = 20;
		osal_start_timerEx(ti->task_id, TH_PERIODIC_MEAS_EVT, INTERVAL_MS(ti->indication_interval));

		break;

	case GATT_TEMP_IND_DISABLED:
		print(LOG_INFO, MODULE "stop temp indication\r\n");

		ti->temp_indication_enable = FALSE;

		break;

	case GATT_IMEAS_NOTI_ENABLED:
		print(LOG_INFO, MODULE "start imeas notification\r\n");

		ti->temp_notification_enable = TRUE;
		ti->notification_interval = 10;
		osal_start_timerEx(ti->task_id, TH_PERIODIC_IMEAS_EVT, INTERVAL_MS(ti->notification_interval));

		break;

	case GATT_IMEAS_NOTI_DISABLED:
		print(LOG_INFO, MODULE "stop imeas notification\r\n");
		ti->temp_notification_enable = FALSE;

		break;

	case GATT_INTERVAL_IND_ENABLED:
		print(LOG_INFO, MODULE "start interval indication\r\n");

		break;

	case GATT_INTERVAL_IND_DISABLED:
		print(LOG_INFO, MODULE "stop interval indication\r\n");

		break;

	case GATT_UNKNOWN:
		print(LOG_INFO, MODULE "unknown gatt access type\r\n");
		break;
	}

	return;
}

static void ther_dispatch_msg(struct ther_info *ti, osal_event_hdr_t *msg)
{
	print(LOG_DBG, "event %d, staus %d\r\n", msg->event, msg->status);
	switch (msg->event) {
	case KEY_CHANGE:
		keyChange_t *kmsg = (keyChange_t *)msg;
		ther_handle_button(kmsg->state, kmsg->keys);
		break;

	case GATT_MSG_EVENT:
		ther_handle_gatt_msg(ti, (gattMsgEvent_t *)msg);
		break;

	case BLE_GATT_ACCESS_EVENT:
		ther_handle_gatt_access_msg(ti, (struct ble_msg *)msg);
		break;

	default:
		break;
	}
}

/*********************************************************************
 * @fn      Thermometer_ProcessEvent
 *
 * @brief   Thermometer Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16 Thermometer_ProcessEvent(uint8 task_id, uint16 events)
{
	struct ther_info *ti = &ther_info;

	if ( events & SYS_EVENT_MSG ) {
		uint8 *msg;

		if ( (msg = osal_msg_receive(ti->task_id)) != NULL ) {
			ther_dispatch_msg(ti, (osal_event_hdr_t *)msg);

			osal_msg_deallocate( msg );
		}

		return (events ^ SYS_EVENT_MSG);
	}

	if(events & TH_PERIODIC_MEAS_EVT) {
		ther_temp_periodic_meas(ti);

		return (events ^ TH_PERIODIC_MEAS_EVT);
	}

	if (events & TH_PERIODIC_IMEAS_EVT) {
		ther_temp_periodic_imeas(ti);

		return (events ^ TH_PERIODIC_IMEAS_EVT);
	}


	return 0;
}


/*********************************************************************
 * @fn      Thermometer_Init
 *
 * @brief   Initialization function for the Thermometer App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void Thermometer_Init(uint8 task_id)
{
	struct ther_info *ti = &ther_info;

	ti->task_id = task_id;

	/* uart init */
	uart_comm_init();

	/* iic init */

	/* oled init */

	/* gpio init */

	/* adc init */

	/* button init */

	ther_ble_init(ti->task_id);

	// Register for all key events - This app will handle all key events
	RegisterForKeys(ti->task_id);
}
