

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
};

static struct ther_info ther_info;



static void ther_handle_msg(osal_event_hdr_t *msg)
{
	print(LOG_DBG, "event %d, staus %d\r\n", msg->event, msg->status);
	switch (msg->event) {
		case KEY_CHANGE:
			ther_handle_button( ((keyChange_t *)msg)->state, ((keyChange_t *)msg)->keys );
			break;

		case GATT_MSG_EVENT:
			ther_handle_gatt_msg( (gattMsgEvent_t *) msg );
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
			ther_handle_msg( (osal_event_hdr_t *)msg );

			osal_msg_deallocate( msg );
		}

		return (events ^ SYS_EVENT_MSG);
	}

	if(events & TH_CCC_UPDATE_EVT) {
		ther_send_temp_indicate(ble_get_gap_handle(), ti->task_id);

		osal_start_timerEx( ti->task_id, TH_CCC_UPDATE_EVT, 3000 );

		return (events ^ TH_CCC_UPDATE_EVT);
	}

	if (events & TH_PERIODIC_IMEAS_EVT) {
//		ther_send_temp_notify(ble_get_gap_handle());
		osal_start_timerEx(ti->task_id, TH_PERIODIC_IMEAS_EVT, 3000);

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

	/* gpio init */

	/* adc init */

	/* button init */

	ther_ble_init(ti->task_id);

	// Register for all key events - This app will handle all key events
	RegisterForKeys( ti->task_id );
}
