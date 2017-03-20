
#include "stdio.h"
#include "MQTTClient.h"

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

void s_send_message(U16 msg_id, void *req, int mod_src, int mod_dst, int sap);

static void MqttDemoTask(task_entry_struct *task_entry_ptr)
{
   ilm_struct ilm;
    kal_uint32 my_index;
    int self = 0;

    kal_get_my_task_index(&my_index);
    self = (int)kal_get_current_task();

	TRACE("MqttDemoTask....");

	yblock_init();

    while (1)
    {
        /* replace by stack sharing, add by mingyin*/
        receive_msg_ext_q_for_stack(task_info_g[task_entry_ptr->task_indx].task_ext_qid, &ilm);
        stack_set_active_module_id(my_index, ilm.dest_mod_id);

		switch (ilm.msg_id)
        {
        case MSG_ID_L4C_NBR_CELL_INFO_REG_CNF:
			TRACE("MSG_ID_L4C_NBR_CELL_INFO_REG_CNF");
			break;
		case MSG_ID_L4C_NBR_CELL_INFO_IND:
			TRACE("MSG_ID_L4C_NBR_CELL_INFO_IND");
//			yblock_dereg_cell_info();
			yblock_handle_cell_info(ilm.local_para_ptr);
			break;
			
        case MSG_ID_YBLOCK_STATUS_ON:
			TRACE("MSG_ID_YBLOCK_STATUS_ON");
			yblock_status_notify(1);
			break;
		case MSG_ID_YBLOCK_STATUS_OFF:
			TRACE("MSG_ID_YBLOCK_STATUS_OFF");
			yblock_status_notify(0);
			break;
		case MSG_ID_YBLOCK_STEP_ON:
			TRACE("MSG_ID_YBLOCK_STEP_ON");
			yblock_step_notify(1);
			break;
		case MSG_ID_YBLOCK_STEP_OFF:
			TRACE("MSG_ID_YBLOCK_STEP_OFF");
			yblock_step_notify(0);
			break;

		case MSG_ID_MQTT_CLOSE:
			TRACE("MSG_ID_MQTT_CLOSE");
			mqtt_close();
			break;

		case MSG_ID_MQTT_START:
			TRACE("MSG_ID_MQTT_START");
			yblock_buzz(300);
			get_register_info(APPKEY, DEVICE_ID);
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

