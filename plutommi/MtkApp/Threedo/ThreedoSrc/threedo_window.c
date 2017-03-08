/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Threedo Studio. (C) 2015
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("THREEDO SOFTWARE")
*  RECEIVED FROM THREEDO AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. THREEDO EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES THREEDO PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE THREEDO SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. THREEDO SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY THREEDO SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND THREEDO'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE THREEDO SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT THREEDO'S OPTION, TO REVISE OR REPLACE THE THREEDO SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  THREEDO FOR SUCH THREEDO SOFTWARE AT ISSUE. 
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#include "MMI_include.h"
#include "AlertScreen.h"
#include "gpiosrvgprot.h"
#include "mmi_rp_app_threedo_def.h"
#include "AlertScreen.h"
#include "threedogprot.h"

/*****************************************************************************
** 本文件主要用来在屏幕显示一些信息, 方便查看
**
**    +=======================+
**    |       status bar      |
**    |-----------------------|
**    |   2015-03-20, 16:36   |
**    |-----------------------|
**    |ADOREC: rec            |  rec/idle/err
**    |FTTM: up,60%,3         |  up/down/none,[progress],[total]
**    |                       |
**    |                       |
**    |-----------------------|
**    |<OK>             <Back>|
**    +=======================+
**
*****************************************************************************/
static WCHAR *tdShowBuffer = NULL;
#define TD_WINDOW_BUFFER_SIZE      1023
#define TD_WINDOW_AUTO_UPDATE_TIMER THREEDO_TIMER_PUBLIC_ID

extern kal_char* release_verno(void);
extern kal_char* release_hal_verno(void);
extern kal_char* release_hw_ver(void);
extern kal_char* build_date_time(void); 
extern kal_char* release_build(void); 
extern kal_char* release_branch(void);
extern kal_char* release_flavor(void);


extern const char *threedo_media_get_state_string(void);
extern const char *threedo_file_trans_get_state_string(void);
void threedo_window_show_status_info(void);

static U8 _td_window_update(void *p)
{
	threedo_window_show_status_info();
}

static void _td_window_auto_update(void)
{
	StartTimer(TD_WINDOW_AUTO_UPDATE_TIMER, 3000, _td_window_auto_update);
	threedo_window_show_status_info();
}

void threedo_window_initialize(void)
{
	if (NULL == tdShowBuffer)
	{
		tdShowBuffer = (WCHAR*)threedo_malloc((TD_WINDOW_BUFFER_SIZE+1)*sizeof(WCHAR));
		memset(tdShowBuffer, 0, (TD_WINDOW_BUFFER_SIZE+1)*sizeof(WCHAR));
	}

	threedo_set_message_protocol(MSG_ID_THREEDO_TASK_SLEEP_REQ, _td_window_update);
}

void threedo_window_show_status_info(void)
{
    mmi_confirm_property_struct arg;

	kal_wsprintf(tdShowBuffer, 
		"REC:%s\n"
		"%s\n",
		threedo_media_get_state_string(),
		threedo_file_trans_get_state_string());

	StartTimer(TD_WINDOW_AUTO_UPDATE_TIMER, 3000, _td_window_auto_update);
	
    srv_backlight_turn_on(SRV_BACKLIGHT_SHORT_TIME);
    mmi_confirm_property_init(&arg, CNFM_TYPE_OK);
    arg.parent_id = GRP_ID_ROOT;
    mmi_confirm_display(tdShowBuffer, MMI_EVENT_INFO, &arg);
    SetLeftSoftkeyFunction(mmi_frm_scrn_close_active_id, KEY_EVENT_UP);
    ChangeCenterSoftkey(0, IMG_GLOBAL_COMMON_CSK);
    SetCenterSoftkeyFunction(mmi_frm_scrn_close_active_id, KEY_EVENT_UP);
    SetRightSoftkeyFunction(mmi_frm_scrn_close_active_id, KEY_EVENT_UP);
}


void threedo_show_version(void)
{
	static WCHAR verno[64];
	
	memset(verno, 0, sizeof(verno));
	kal_wsprintf(verno, "%s", build_date_time());
    TD_TRACE("[secure] build: %s.", build_date_time());
	mmi_popup_display_simple(verno, MMI_EVENT_SUCCESS, GRP_ID_ROOT, NULL);
}

