
#include "MMI_include.h"
#include "kal_public_defs.h"
#include "stack_common.h"  
#include "stack_config.h"
#include "stack_msgs.h"
#include "task_config.h"
#include "syscomp_config.h"
#include "stack_ltlcom.h"

#include "threedogprot.h"
#include "threedo_ftm.h"

static void ThreedoTask(task_entry_struct *task_entry_ptr)
{
    ilm_struct ilm;
    kal_uint32 my_index;
    int self = 0;

    kal_get_my_task_index(&my_index);
    self = (int)kal_get_current_task();

	TD_TRACE("ThreedoTask....");

    while (1)
    {
        /* replace by stack sharing, add by mingyin*/
        receive_msg_ext_q_for_stack(task_info_g[task_entry_ptr->task_indx].task_ext_qid, &ilm);
        stack_set_active_module_id(my_index, ilm.dest_mod_id);

        switch (ilm.msg_id)
        {		
        case MSG_ID_THREEDO_TASK_START_REQ:
			TD_TRACE("[task] MSG_ID_THREEDO_TASK_START_REQ");
			threedo_send_message(MSG_ID_MQTT_START, NULL, MOD_MMI, MOD_MQTT, 0);
			break;
			
		case MSG_ID_THREEDO_TASK_STOP_REQ:
			TD_TRACE("[task] MSG_ID_THREEDO_TASK_STOP_REQ");
			threedo_file_trans_free_all();
			break;

		case MSG_ID_THREEDO_TIMER_EXPERIOD_IND:
			threedo_file_trans_timeout_proc(ilm.local_para_ptr);
			break;
			
        case MSG_ID_THREEDO_FILE_TRANS_REQ:
			threedo_file_trans_process_ready((threedo_ft_proc_req *)ilm.local_para_ptr);
		case MSG_ID_THREEDO_FILE_UPLOAD_REQ:
	//	case MSG_ID_THREEDO_FILE_DOWNLOAD_REQ:
			threedo_file_trans_process();
			break;

		case MSG_ID_THREEDO_FILE_TRANS_FINISH_IND:
			{
				char buf[100];
				U8 act = ((threedo_ft_finish_ind*)ilm.local_para_ptr)->action;
				memset(buf, 0, 100);
				
				TD_TRACE("[task] trans finish, action=%d.", ((threedo_ft_finish_ind*)ilm.local_para_ptr)->action);
				app_ucs2_strcat((WCHAR*)buf, (const char*)threedo_get_file_name_from_path(((threedo_ft_finish_ind*)ilm.local_para_ptr)->path));
				
				if (act != TD_TT_ACTION_IS_HEARTBEAT) {
					threedo_send_message(MSG_ID_MQTT_PUBLISH, (void*)buf, 
						MOD_THREEDO, MOD_MQTT, 0);
				}

				break;
			}

		case MSG_ID_THREEDO_FILE_DOWNLOAD_REQ:
			{
				char buf[100];
				char tmp[100];
				memset(buf, 0, 100);
				memset(tmp, 0, 100);
				kal_wsprintf((WCHAR*)tmp, "%s", "heart.beat");
				app_ucs2_strcat(buf, ilm.local_para_ptr);

				if (app_ucs2_strcmp(buf, tmp) == 0) {
					TD_TRACE("<===heart beat  ===>");
					break;
				}

				kal_wsprintf((WCHAR*)tmp, "%s", ".amr");
				if (app_ucs2_strstr(buf, tmp) == NULL) {
					TD_TRACE("<===not amr filename  ===>");
					break;
				}
			
				TD_TRACE("<===download req from mqqt ===>");

				threedo_download_start((WCHAR*)buf);
/*
				threedo_send_file_trans_req(
					(const WCHAR *)buf, 
					0,
					0,
					TD_TT_ACTION_IS_DOWNLOAD);
*/
				break;
			}
			
        default:
		threedo_file_trans_socket_event_handle(&ilm);
            break;
        }

        free_ilm(&ilm);
    }
}



/*************************************************************************
* FUNCTION                                                            
*	threedo_create
*
* DESCRIPTION                                                           
*	This function implements Threedo entity's create handler.
*
* PARAMETERS
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_bool threedo_create(comptask_handler_struct **handle)
{
	static const comptask_handler_struct idle_handler_info = 
	{
		ThreedoTask,			/* task entry function */
		NULL,			/* task initialization function */
		NULL,		/* task configuration function */
		NULL,			/* task reset handler */
		NULL			/* task termination handler */
	};

	*handle = (comptask_handler_struct *)&idle_handler_info;


	return KAL_TRUE;
}


