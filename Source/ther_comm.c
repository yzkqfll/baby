
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

#include "ther_temp.h"
#include "ther_comm.h"

#define MODULE "[THER COMM] "

// flags for simulated measurements
static const uint8 ther_flags[] =
{
  THERMOMETER_FLAGS_CELCIUS | THERMOMETER_FLAGS_TIMESTAMP |THERMOMETER_FLAGS_TYPE,
  THERMOMETER_FLAGS_CELCIUS | THERMOMETER_FLAGS_TIMESTAMP,
  THERMOMETER_FLAGS_CELCIUS,
  THERMOMETER_FLAGS_FARENHEIT,
  THERMOMETER_FLAGS_FARENHEIT | THERMOMETER_FLAGS_TIMESTAMP,
  THERMOMETER_FLAGS_FARENHEIT | THERMOMETER_FLAGS_TIMESTAMP | THERMOMETER_FLAGS_TYPE,
  0x00
};

#define THER_NOTIFY_FLAG (THERMOMETER_FLAGS_CELCIUS | THERMOMETER_FLAGS_TYPE)

static unsigned char encap_temp_buf(unsigned char flag, unsigned int temp, unsigned char *buf)
{
	unsigned char *start_start = buf;

	/* flag */
	*buf++ = flag;

	if (flag & THERMOMETER_FLAGS_FARENHEIT)
		temp  = (temp * 9  /5) + 320;

	temp = 0xFF000000 | temp;

	/* temp */
	osal_buffer_uint32(buf, temp);
	buf += 4;

    //timestamp
    if (flag & THERMOMETER_FLAGS_TIMESTAMP) {
      UTCTimeStruct time;

      // Get time structure from OSAL
      osal_ConvertUTCTime( &time, osal_getClock() );

      *buf++ = (time.year & 0x00FF);
      *buf++ = (time.year & 0xFF00)>>8;
      //*p++ = time.year;
      //*p++;

      *buf++ = time.month;
      *buf++ = time.day;
      *buf++ = time.hour;
      *buf++ = time.minutes;
      *buf++ = time.seconds;
    }

	if(flag & THERMOMETER_FLAGS_TYPE)
	{
		uint8 type;
		Thermometer_GetParameter( THERMOMETER_TYPE, &type );
		*buf++ =  type;
	}

	return buf - start_start;
}

void ther_send_temp_notify(uint16 connect_handle)
{
	attHandleValueNoti_t notify;
	unsigned int temp = get_current_temp();

	notify.len = encap_temp_buf(THER_NOTIFY_FLAG, temp, notify.value);
//	notify.handle = THERMOMETER_IMEAS_VALUE_POS;

	print(LOG_DBG, MODULE "send notify(temp %d)\r\n", temp);

	Thermometer_IMeasNotify( connect_handle, &notify);

	return;
}

void ther_send_temp_indicate(uint16 connect_handle, unsigned char task_id)
{
	attHandleValueInd_t indicate;
	unsigned int temp = get_current_temp();

	indicate.len = encap_temp_buf(THER_NOTIFY_FLAG, temp, indicate.value);
	indicate.handle = THERMOMETER_TEMP_VALUE_POS;

	print(LOG_DBG, MODULE "send indicate(temp %d)\r\n", temp);

	Thermometer_TempIndicate( connect_handle, &indicate, task_id);

	return;
}

void ther_handle_gatt_msg(gattMsgEvent_t *pMsg)
{
	//Measurement Indication Confirmation
	if( pMsg->method ==ATT_HANDLE_VALUE_CFM)
	{
//		thermometerSendStoredMeas();
	}

	if ( pMsg->method == ATT_HANDLE_VALUE_NOTI ||
			pMsg->method == ATT_HANDLE_VALUE_IND )
	{
		timeAppIndGattMsg( pMsg );
	}
	else if ( pMsg->method == ATT_READ_RSP ||
			pMsg->method == ATT_WRITE_RSP )
	{
	}
	else
	{
	}
}
