
#include "stdio.h"
#include "MQTTClient.h"
#include "gpio_drv.h"
#include "gpio_def.h"
#include "kal_release.h"
//#include "gpio_sw.h"
#include "Eint.h"

const char *APPKEY = "56a0a88c4407a3cd028ac2fe";
const char *DEVICE_ID = "2000000001";

static S32 socket_event_handle(ilm_struct *ilm)
{
	S32 proced = 1;
	
	switch(ilm->msg_id)
	{
	case MSG_ID_APP_SOC_GET_HOST_BY_NAME_IND:
		host_name_cb((void *)ilm->local_para_ptr);
		break;
	case MSG_ID_APP_SOC_NOTIFY_IND:
		event_cb((void *)ilm->local_para_ptr);
		break;
	default:
		proced = 0;
		break;
	}

	return proced;
}

#if 1
extern const kal_uint8 TOUCH_PANEL_EINT_NO;
void s_send_message(U16 msg_id, void *req, int mod_src, int mod_dst, int sap);

static void eint_test_callback()
{
	s_send_message(0x2345, 0, MOD_DRV_HISR, MOD_MQTT, 0);
	//TRACE("======eint_test_callback");
	//EINT_UnMask(TOUCH_PANEL_EINT_NO);
}
#endif

static kal_timerid mTestTimer = 0;

//#define __GPIO_MODE__
#if !defined(__EDGE_MODE__)
static kal_bool mIntLevel = KAL_FALSE;
#endif
static const kal_uint8 gpio0_pin = 0;
static const kal_uint8 gpio1_pin = 1;

static void mTestTimerCallback(void *arg)
{
	static kal_uint32 count = 0;
	count ++;
	TRACE("my timer is expired. %d", count);
	GPIO_WriteIO(count%2, gpio1_pin);
}

static void InterruptCallback(void)
{
	if (mIntLevel)
	{
		
	}
	else
	{
		
	}
	
	mIntLevel = !mIntLevel; 
	EINT_SW_Debounce_Modify(TOUCH_PANEL_EINT_NO,10);
	EINT_Set_Polarity(TOUCH_PANEL_EINT_NO, mIntLevel);
	s_send_message(0x2345, 0, MOD_DRV_HISR, MOD_MQTT, 0);
}

static void InterruptConfig(void)
{
#if defined(__EDGE_MODE__)
	EINT_Mask(TOUCH_PANEL_EINT_NO);
	EINT_Registration(TOUCH_PANEL_EINT_NO, KAL_FALSE, KAL_FALSE, eint_test_callback, KAL_FALSE);
	EINT_Set_Sensitivity(TOUCH_PANEL_EINT_NO, KAL_FALSE);
	EINT_Set_Polarity(TOUCH_PANEL_EINT_NO, KAL_FALSE);
	EINT_UnMask(TOUCH_PANEL_EINT_NO);
#elif defined(__GPIO_MODE__)
	GPIO_ModeSetup(gpio0_pin, 0);
	GPIO_InitIO(OUTPUT, gpio0_pin);
	GPIO_PullenSetup(gpio0_pin, 1);
	GPIO_WriteIO(1,gpio0_pin);
#else

	GPIO_ModeSetup(gpio1_pin, 0);
	GPIO_InitIO(OUTPUT, gpio1_pin);
	GPIO_PullenSetup(gpio1_pin, 1);
	GPIO_WriteIO(1,gpio1_pin);
	
	EINT_Mask(TOUCH_PANEL_EINT_NO);
	GPIO_ModeSetup(gpio0_pin, 1);
	GPIO_InitIO(INPUT, gpio0_pin);
	GPIO_PullenSetup(gpio0_pin, 1);
	EINT_Registration(TOUCH_PANEL_EINT_NO, KAL_TRUE, mIntLevel, InterruptCallback, KAL_TRUE);
	EINT_Set_Sensitivity(TOUCH_PANEL_EINT_NO, LEVEL_SENSITIVE);
	EINT_Set_Polarity(TOUCH_PANEL_EINT_NO, mIntLevel);
	EINT_SW_Debounce_Modify(TOUCH_PANEL_EINT_NO,10);
	EINT_UnMask(TOUCH_PANEL_EINT_NO);
#endif
}

static void MqttDemoTask(task_entry_struct *task_entry_ptr)
{
   ilm_struct ilm;
    kal_uint32 my_index;
    int self = 0;
	static int first = 1;
	kal_bool inited = KAL_FALSE;

    kal_get_my_task_index(&my_index);
    self = (int)kal_get_current_task();
	mTestTimer = kal_create_timer("mqtt");

	TRACE("MqttDemoTask....");

#if 0
	//psEintLevel = KAL_FALSE;
	//EINT_Mask(PSALS_EINT_NO);
	//EINT_Registration(PSALS_EINT_NO,KAL_FALSE,psEintLevel,PsalsEintHisr,KAL_FALSE);
	//EINT_Set_Sensitivity(PSALS_EINT_NO, KAL_FALSE);//false:level int,true:edge
	//EINT_Set_Polarity(PSALS_EINT_NO, psEintLevel);

	//EINT_Set_Sensitivity(AST_WAKEUP_EINT_NO, EDGE_SENSITIVE);
	//EINT_Set_Polarity(AST_WAKEUP_EINT_NO, KAL_TRUE);
	//EINT_Registration(AST_WAKEUP_EINT_NO, KAL_FALSE, KAL_TRUE, pEintParam->fWakeUpCB, KAL_TRUE);

	EINT_Mask(TOUCH_PANEL_EINT_NO);
    EINT_Registration(TOUCH_PANEL_EINT_NO, KAL_FALSE, KAL_FALSE, eint_test_callback, KAL_FALSE);
	EINT_Set_Sensitivity(TOUCH_PANEL_EINT_NO, KAL_FALSE);
    EINT_Set_Polarity(TOUCH_PANEL_EINT_NO, KAL_FALSE);
	EINT_UnMask(TOUCH_PANEL_EINT_NO);
#endif

    while (1)
    {
        /* replace by stack sharing, add by mingyin*/
        receive_msg_ext_q_for_stack(task_info_g[task_entry_ptr->task_indx].task_ext_qid, &ilm);
        stack_set_active_module_id(my_index, ilm.dest_mod_id);

		switch (ilm.msg_id)
        {
		case 0x2345:
			TRACE("recived interrupt message.");
			break;

		case MSG_ID_MQTT_CLOSE:
			TRACE("======MSG_ID_MQTT_CLOSE");
			//mqtt_close();
			break;

		case MSG_ID_MQTT_START:
			TRACE("MSG_ID_MQTT_START");
			//get_register_info(APPKEY, DEVICE_ID);
			if (!inited)
			{
				inited = KAL_TRUE;
				kal_set_timer(mTestTimer, mTestTimerCallback, NULL, 300, 300);
				InterruptConfig();
			}
			break;

		case MSG_ID_MQTT_KEEPALIVE:
			MQTTkeepalive_start();
			break;

		case MSG_ID_MQTT_CONN:
			TRACE("MSG_ID_MQTT_CONN");
			mqtt_conn_start();
			break;

		case MSG_ID_MQTT_SET_ALIAS:
			TRACE("MSG_ID_MQTT_SET_ALIAS");
			publish_message(",yali", DEVICE_ID, strlen(DEVICE_ID));
			break;
			
		default:
			socket_event_handle(&ilm);
			break;
		}
		
        free_ilm(&ilm);		
	}
}


void s_send_message(U16 msg_id, void *req, int mod_src, int mod_dst, int sap)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    ilm_struct *ilm_send;
	
    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    ilm_send = allocate_ilm(mod_src);   
    ilm_send->src_mod_id = mod_src;
    ilm_send->dest_mod_id = (module_type)mod_dst;
    ilm_send->sap_id = sap;
    ilm_send->msg_id = (msg_type) msg_id;
    ilm_send->local_para_ptr = (local_para_struct*) req;
    ilm_send->peer_buff_ptr = (peer_buff_struct*) NULL;    
    msg_send_ext_queue(ilm_send);    
}

kal_bool MqttDemo_create(comptask_handler_struct **handle)
{
	static const comptask_handler_struct idle_handler_info = 
	{
		MqttDemoTask,			/* task entry function */
		NULL,			/* task initialization function */
		NULL,		/* task configuration function */
		NULL,			/* task reset handler */
		NULL			/* task termination handler */
	};

	*handle = (comptask_handler_struct *)&idle_handler_info;

	return KAL_TRUE;
}



