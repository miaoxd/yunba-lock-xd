#include "MQTTClient.h"
#include "gpio_def.h"
#include "gpio_drv.h"
#include "kal_release.h"
#include "stdio.h"
#ifndef _WIN32
#include "gpio_sw.h"
#endif
#include "Eint.h"
#include "nbr_public_struct.h"

#define GPIO_MOTOR GPIO_PORT_2
#define GPIO_BUZZER GPIO_PORT_3
#define SW_DEBOUNCE 3

#define MAX_CELL_INFO 6

typedef enum {
    LOCK_STATUS_LOCKED = 0,
    LOCK_STATUS_LOCKING,
    LOCK_STATUS_UNLOCKED,
    LOCK_STATUS_UNLOCKING
} LOCK_STATUS;

typedef struct {
    kal_uint16 mcc;
    kal_uint16 mnc;
    kal_uint16 lac;
    kal_uint16 ci;
    kal_uint8 sig;
} CELL_INFO;

extern const char* DEVICE_ID;

static const kal_uint8 CHECK_STATUS_EINT_NO = 0;
static const kal_uint8 CHECK_STEP_EINT_NO = 1;

static kal_bool g_eint_level_status = LEVEL_LOW;
static kal_bool g_eint_level_step = LEVEL_LOW;

static LOCK_STATUS g_lock_status = LOCK_STATUS_LOCKED;
static int g_lock_step = 0;
static int g_buzzing = 0;

static int g_cell_info_cnt = 0;
static CELL_INFO g_cell_info[MAX_CELL_INFO];
static char g_buf[1024];
static char g_buf_cell[512];

void yblock_buzz(int mills);
void yblock_report();

static void yblock_stop_buzz()
{
    GPIO_WriteIO(0, GPIO_BUZZER);
    g_buzzing = 0;
}

static void yblock_start_motor() { GPIO_WriteIO(0, GPIO_MOTOR); }

static void yblock_stop_motor() { GPIO_WriteIO(1, GPIO_MOTOR); }

static void yblock_eint_status(void)
{
	EINT_Mask(CHECK_STATUS_EINT_NO);
    if (g_eint_level_status) {
        s_send_message(MSG_ID_YBLOCK_STATUS_OFF, 0, MOD_DRV_HISR, MOD_MQTT, 0);
    } else {
        s_send_message(MSG_ID_YBLOCK_STATUS_ON, 0, MOD_DRV_HISR, MOD_MQTT, 0);
    }
    g_eint_level_status = !g_eint_level_status;
//    EINT_SW_Debounce_Modify(CHECK_STATUS_EINT_NO, SW_DEBOUNCE);
    EINT_Set_Polarity(CHECK_STATUS_EINT_NO, g_eint_level_status);
	EINT_UnMask(CHECK_STATUS_EINT_NO);
}

static void yblock_eint_step(void)
{
	EINT_Mask(CHECK_STEP_EINT_NO);
    if (g_eint_level_step) {
        s_send_message(MSG_ID_YBLOCK_STEP_OFF, 0, MOD_DRV_HISR, MOD_MQTT, 0);
    } else {
        s_send_message(MSG_ID_YBLOCK_STEP_ON, 0, MOD_DRV_HISR, MOD_MQTT, 0);
    }

    g_eint_level_step = !g_eint_level_step;
//    EINT_SW_Debounce_Modify(CHECK_STEP_EINT_NO, 10);
    EINT_Set_Polarity(CHECK_STEP_EINT_NO, g_eint_level_step);
	EINT_UnMask(CHECK_STEP_EINT_NO);
}

static void yblock_gen_cell_json()
{
    char* pos = g_buf_cell;
    int size = sizeof(g_buf_cell);
    int len;
    int i = 0;

    for (i = 0; i < g_cell_info_cnt; i++) {
        len = _snprintf(pos, size,
            "%s{\"mcc\":%d,\"mnc\":%d,\"lac\":%d,\"ci\":%d,\"sig\":%d}",
            i ? "," : "", g_cell_info[i].mcc, g_cell_info[i].mnc,
            g_cell_info[i].lac, g_cell_info[i].ci, g_cell_info[i].sig);
        pos += len;
        size -= len;
    }
}

static int yblock_gen_report_json(char* buf, int size, const char* lock,
    int battery, const char* charge,
    const char* cell)
{
    return _snprintf(buf, size,
        "{\"lock\":%s,\"battery\":%d,\"charge\":%s,\"cell\":[%s]}",
        lock, battery, charge, cell);
}

void yblock_init()
{
#ifndef WIN32
    EINT_Mask(CHECK_STATUS_EINT_NO);
    EINT_Registration(CHECK_STATUS_EINT_NO, KAL_FALSE, g_eint_level_status,
        yblock_eint_status, KAL_TRUE);
    EINT_Set_Sensitivity(CHECK_STATUS_EINT_NO, LEVEL_SENSITIVE);
    EINT_Set_Polarity(CHECK_STATUS_EINT_NO, g_eint_level_status);
//    EINT_SW_Debounce_Modify(CHECK_STATUS_EINT_NO, SW_DEBOUNCE);
    EINT_UnMask(CHECK_STATUS_EINT_NO);

    EINT_Mask(CHECK_STEP_EINT_NO);
    EINT_Registration(CHECK_STEP_EINT_NO, KAL_FALSE, g_eint_level_step,
        yblock_eint_step, KAL_TRUE);
    EINT_Set_Sensitivity(CHECK_STEP_EINT_NO, LEVEL_SENSITIVE);
    EINT_Set_Polarity(CHECK_STEP_EINT_NO, g_eint_level_step);
//    EINT_SW_Debounce_Modify(CHECK_STEP_EINT_NO, SW_DEBOUNCE);
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
    U8 battery = YxAppBatteryLevel();
    const char* lock = (g_lock_status == LOCK_STATUS_LOCKED ? "true" : "false");
    const char* charge = (srv_charbat_is_charger_connect() ? "true" : "false");
    int len;

    yblock_gen_cell_json();

    len = yblock_gen_report_json(g_buf, sizeof(g_buf), lock, battery, charge,
        g_buf_cell);

    publish_message(DEVICE_ID, g_buf, len);
}

void yblock_status_notify(int value)
{
    if (value) {
        if (g_lock_status == LOCK_STATUS_UNLOCKED) {
			g_lock_status = LOCK_STATUS_LOCKING;
            yblock_start_motor();
            g_lock_step = 0;
            yblock_buzz(300);
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
				g_lock_status = LOCK_STATUS_UNLOCKED;
                yblock_stop_motor();
                yblock_report();
                yblock_buzz(300);
                TRACE("yblock_step_notify: unlock finished");
            }
        } else if (g_lock_status == LOCK_STATUS_LOCKING) {
            if (g_lock_step == 1) {
				g_lock_status = LOCK_STATUS_LOCKED;
                yblock_stop_motor();
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

void yblock_reg_cell_info()
{
    s_send_message(MSG_ID_L4C_NBR_CELL_INFO_REG_REQ, 0, MOD_MQTT, MOD_L4C, 0);
}

void yblock_dereg_cell_info()
{
    s_send_message(MSG_ID_L4C_NBR_CELL_INFO_DEREG_REQ, 0, MOD_MQTT, MOD_L4C, 0);
}

void yblock_handle_cell_info(l4c_nbr_cell_info_ind_struct* msg_ptr)
{
    gas_nbr_cell_info_struct* nci;
    gas_nbr_cell_meas_struct* meas;
    int i = 0;

    nci = &msg_ptr->ps_nbr_cell_info_union.gas_nbr_cell_info;

	TRACE("yblock_handle_cell_info: get: %d", nci->nbr_cell_num);
    g_cell_info_cnt = MIN(nci->nbr_cell_num, MAX_CELL_INFO);
	TRACE("yblock_handle_cell_info: use: %d", g_cell_info_cnt);

	
    if (msg_ptr->is_nbr_info_valid) {
        for (i = 0; i < g_cell_info_cnt; i++) {
            g_cell_info[i].mcc = nci->nbr_cell_info[i].gci.mcc;
            g_cell_info[i].mnc = nci->nbr_cell_info[i].gci.mnc;
            g_cell_info[i].lac = nci->nbr_cell_info[i].gci.lac;
            g_cell_info[i].ci = nci->nbr_cell_info[i].gci.ci;

            meas = &nci->nbr_meas_rslt
                        .nbr_cells[nci->nbr_cell_info[i].nbr_meas_rslt_index];
            g_cell_info[i].sig = meas->rxlev;
        }
    }
}

