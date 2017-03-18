#include "stdio.h"
#include "MQTTClient.h"
#include "gpio_drv.h"
#include "gpio_def.h"
#include "kal_release.h"
#ifndef _WIN32
#include "gpio_sw.h"
#endif
#include "Eint.h"

#define GPIO_MOTOR GPIO_PORT_2
#define GPIO_BUZZER GPIO_PORT_3

typedef enum {
  LOCK_STATUS_LOCKED = 0,
  LOCK_STATUS_LOCKING,
  LOCK_STATUS_UNLOCKED,
  LOCK_STATUS_UNLOCKING
} LOCK_STATUS;

extern const char *DEVICE_ID;

static const kal_uint8 CHECK_STATUS_EINT_NO = 0;
static const kal_uint8 CHECK_STEP_EINT_NO = 1;

static kal_bool g_eint_level_status = LEVEL_LOW;
static kal_bool g_eint_level_step = LEVEL_LOW;


static LOCK_STATUS g_lock_status = LOCK_STATUS_LOCKED;
static int g_lock_step = 0;
static int g_buzzing = 0;

void yblock_buzz(int mills);
void yblock_report();


static void yblock_stop_buzz()
{
	GPIO_WriteIO(0, GPIO_BUZZER);
	g_buzzing = 0;
}

static void yblock_start_motor()
{
	GPIO_WriteIO(0, GPIO_MOTOR);
}

static void yblock_stop_motor()
{
	GPIO_WriteIO(1, GPIO_MOTOR);
}

static void yblock_eint_status(void)
{
	if (g_eint_level_status) {
		s_send_message(MSG_ID_YBLOCK_STATUS_OFF, 0, MOD_DRV_HISR, MOD_MQTT, 0);
	} else {
		s_send_message(MSG_ID_YBLOCK_STATUS_ON, 0, MOD_DRV_HISR, MOD_MQTT, 0);
	}
	g_eint_level_status = !g_eint_level_status; 
	EINT_SW_Debounce_Modify(CHECK_STATUS_EINT_NO, 10);
	EINT_Set_Polarity(CHECK_STATUS_EINT_NO, g_eint_level_status);
}

static void yblock_eint_step(void)
{
	if (g_eint_level_step) {
		s_send_message(MSG_ID_YBLOCK_STEP_OFF, 0, MOD_DRV_HISR, MOD_MQTT, 0);
	} else {
		s_send_message(MSG_ID_YBLOCK_STEP_ON, 0, MOD_DRV_HISR, MOD_MQTT, 0);
	}

	g_eint_level_step = !g_eint_level_step; 
	EINT_SW_Debounce_Modify(CHECK_STEP_EINT_NO, 10);
	EINT_Set_Polarity(CHECK_STEP_EINT_NO, g_eint_level_step);
}


void yblock_init()
{
#ifndef WIN32
	EINT_Mask(CHECK_STATUS_EINT_NO);
	EINT_Registration(CHECK_STATUS_EINT_NO, KAL_TRUE, g_eint_level_status, yblock_eint_status, KAL_TRUE);
	EINT_Set_Sensitivity(CHECK_STATUS_EINT_NO, LEVEL_SENSITIVE);
	EINT_Set_Polarity(CHECK_STATUS_EINT_NO, g_eint_level_status);
	EINT_SW_Debounce_Modify(CHECK_STATUS_EINT_NO, 10);
	EINT_UnMask(CHECK_STATUS_EINT_NO);

	EINT_Mask(CHECK_STEP_EINT_NO);
	EINT_Registration(CHECK_STEP_EINT_NO, KAL_TRUE, g_eint_level_step, yblock_eint_step, KAL_TRUE);
	EINT_Set_Sensitivity(CHECK_STEP_EINT_NO, LEVEL_SENSITIVE);
	EINT_Set_Polarity(CHECK_STEP_EINT_NO, g_eint_level_step);
	EINT_SW_Debounce_Modify(CHECK_STEP_EINT_NO, 10);
	EINT_UnMask(CHECK_STEP_EINT_NO);
#endif
	yblock_buzz(1000);

}

void yblock_unlock()
{
	if (g_lock_status != LOCK_STATUS_LOCKED) {
		TRACE("lock_unlock: not locked now");
		return;
	}
	g_lock_status = LOCK_STATUS_UNLOCKING;
	g_lock_step = 0;
	yblock_start_motor();
}

void yblock_buzz(int mills)
{
	if (!g_buzzing) {
		GPIO_WriteIO(1, GPIO_BUZZER);
		StartTimer(YBLOCK_BUZZ_TIMER, mills, yblock_stop_buzz);
		g_buzzing = 1;
	}
}

void yblock_report()
{
	char buf[256];
	U8 battery = YxAppBatteryLevel();
	const char *lock = (g_lock_status == LOCK_STATUS_LOCKED ? "true" : "false");
	const char *charge = (srv_charbat_is_charger_connect() ? "true" : "false");

	_snprintf(buf, sizeof(buf), "{\"lock\":%s,\"battery\":%d,\"charge\":%s}", lock, battery, charge);
	publish_message(DEVICE_ID, buf, strlen(buf));
}

void yblock_status_notify(int value)
{
	if (value) {
		if (g_lock_status == LOCK_STATUS_UNLOCKED) {
			yblock_start_motor();
			g_lock_status = LOCK_STATUS_LOCKING;
			g_lock_step = 0;
			TRACE("yblock_status_notify: locking");
		}
	}
}

void yblock_step_notify(int value)
{
	if (!value) {
		if (g_lock_status == LOCK_STATUS_UNLOCKING) {
			if (g_lock_step == 0) {
				g_lock_step = 1;
				TRACE("yblock_step_notify: unlock step: 1");
			}
		} else if (g_lock_status == LOCK_STATUS_LOCKING) {
			if (g_lock_step == 0) {
				g_lock_step = 1;
				TRACE("yblock_step_notify: lock step: 1");
			}
		}
	} else {
		if (g_lock_status == LOCK_STATUS_UNLOCKING) {
			if (g_lock_step == 1) {
				yblock_stop_motor();
				g_lock_status = LOCK_STATUS_UNLOCKED;
				yblock_report();
				yblock_buzz(300);
				TRACE("yblock_step_notify: unlock finished");
			}
		} else if (g_lock_status == LOCK_STATUS_LOCKING) {
			if (g_lock_step == 1) {
				yblock_stop_motor();
				g_lock_status = LOCK_STATUS_LOCKED;
				yblock_report();
				yblock_buzz(400);
				TRACE("yblock_step_notify: lock finished");
			}
		}
	}
}

void yblock_adjust()
{
	yblock_start_motor();
	kal_sleep_task(60);
	yblock_stop_motor();
}

