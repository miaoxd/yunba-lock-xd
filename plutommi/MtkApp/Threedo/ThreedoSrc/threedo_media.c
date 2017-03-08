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

#include "fs_gprot.h"
#include "mdi_audio.h"
#include "app_str.h"

#include "mmi_rp_app_threedo_def.h"

#include "ThreedoGprot.h"
#include "threedo_errno.h"
#include "threedo_media.h"
#include "threedo_ftm.h"

extern U32 sh_ModisCreateJPEG(WCHAR *filepath);
extern U32 sh_ModisCreateAVI(WCHAR *filepath);
extern U32 sh_ModisCreateWAV(WCHAR *filepath);
extern U32 sh_ModisCreateAMR(WCHAR *filepath);

#define THREEDO_MEDIA_AUDIO_RECORD_LIMITED_TIMER  THREEDO_TIMER_PUBLIC_ID

static U32 _td_audio_record_check_file(const WCHAR *fullname, S32 del_zero)
{
	FS_HANDLE handle;
	U32 file_size = 0;
	
	handle = FS_Open(fullname, FS_READ_ONLY);
    if (handle >= FS_NO_ERROR)
    {
       FS_GetFileSize(handle, &file_size);
       FS_Close(handle);
	   
	#if defined (__MTK_TARGET__)     
        if(file_size == 0 && del_zero)
        {
            FS_Delete(fullname);
        }
	#endif	
    }

	return file_size;
}

static WCHAR *_td_audio_record_get_path_name(WCHAR *outname, WCHAR **fileNamePosInBuf, S32 drive, char *flName)
{
	S32 fd;
	int i = 0;
	WCHAR *pos;
	
	if (NULL == outname)
		return NULL;

	kal_wsprintf(outname, "%c:\\%s\\%s", drive, THREEDO_FOLDER, flName);

	if (!threedo_create_path_multi_ex((const WCHAR *)outname))
	{
		TD_TRACE("[media] create path failed.");
		return NULL;
	}

	//mdi_audio_get_record_param(mmi_sndrec_get_rec_type(), (MDI_AUDIO_REC_QUALITY_ENUM)quality, &rec_param)
	pos = outname + app_ucs2_strlen((const S8 *)outname);
	*pos++ = '\\';
	
	if (fileNamePosInBuf)
	{
		*fileNamePosInBuf = pos;
	}
	
	do {
		kal_wsprintf(pos, "Ado%x%d.%s",
			threedo_get_system_tick(),
			i++,
		#if defined(THREEDO_ADO_QUALITY_LEVEL) && (THREEDO_ADO_QUALITY_LEVEL == 1)
			"wav"
		#else
			"amr"
		#endif
			);

		//检查文件是否存在
		fd = FS_Open((const WCHAR*)outname, FS_READ_ONLY);
		if (fd < FS_NO_ERROR)
		{
			break;
		}
		else
		{
			FS_Close(fd);
		}
	}while(1);
	
	return outname;
}

static void _td_audio_record_callback(int result, void *data)
{
	struct sric_context *pCntx = (struct sric_context *)data;
		
	TD_TRACE("[media] audio record callback(result=%d).", result);
	TD_ASSERT(NULL != pCntx);
	
	if (pCntx->status == SRIC_STATE_RECORDING)
	{
		pCntx->status = SRIC_STATE_IDLE;

        //检查文件大小并调用发送数据的函数
		switch (result)
		{
        case MDI_AUDIO_SUCCESS:
        case MDI_AUDIO_TERMINATED:
        case MDI_AUDIO_DISC_FULL:
		#if defined(WIN32)
			sh_ModisCreateAMR((WCHAR*)pCntx->filepath);
		#endif
		
        default:
        	break;
        }

		if (pCntx->callback)
		{
			pCntx->callback((const WCHAR *)pCntx->filepath,
				(const WCHAR *)pCntx->fileNamePostionIn_filepath);
		}
	}	

	threedo_mfree((void *)pCntx);
}

S32 threedo_audio_record(U32 timeMs, U32 sizeKb, S32 drive, void (*callback)(const WCHAR *abs_path, const WCHAR *fname))
{
	MDI_RESULT result;
	struct sric_context *sricContext;

	if (mdi_audio_is_recording() || mdi_audio_is_playing(MDI_AUDIO_PLAY_ANY))
		return TD_ERRNO_MODULE_BUSING;
	
	
	if (0 == timeMs && 0 == sizeKb)
	{
		return TD_ERRNO_PARAM_ILLEGAL;
	}
	else if (!threedo_is_drive_usable(drive))
	{
		return TD_ERRNO_CARD_NOT_USABLE;
	}

	sricContext = (struct sric_context *)threedo_malloc(sizeof(struct sric_context));
	if (NULL != _td_audio_record_get_path_name(sricContext->filepath, 
				&sricContext->fileNamePostionIn_filepath,
				drive,
				THREEDO_AUDIO_FOLER))
	{
		result = mdi_audio_start_record_with_limit(
	            (void*)sricContext->filepath,
	        #if defined(THREEDO_ADO_QUALITY_LEVEL) && (THREEDO_ADO_QUALITY_LEVEL == 1)
				MDI_FORMAT_WAV,
			#else
	         	MDI_FORMAT_AMR,
	        #endif
	            MDI_AUDIO_REC_QUALITY_AUTO,
	            _td_audio_record_callback,
	            (void *)sricContext,
	            timeMs,
	            sizeKb*1024);

		TD_TRACE("[media] audio record start(result=%d, cb=0x%x).", result, callback);
		if (MDI_AUDIO_SUCCESS == result)
		{
			sricContext->status = SRIC_STATE_RECORDING;
			sricContext->callback = callback;
			return TD_ERRNO_NO_ERROR;
		}
	}
	
	threedo_mfree((void *)sricContext);
	return TD_ERRNO_ADOREC_FAILED;
}

S32 threedo_audio_record_stop(void)
{
	//因为最大录音时间只能是20秒
	if (mdi_audio_is_recording())
	{
		mdi_audio_stop_record();
	}
	
	return TD_ERRNO_NO_ERROR;
}



static void _td_audio_play_callback(mdi_result result, void* user_data)
{
	struct sric_context *pCntx = (struct sric_context *)user_data;

	TD_TRACE("[media] audio play callback(result=%d).", result);
	TD_ASSERT(NULL != pCntx);
	
	pCntx->status = SRIC_STATE_IDLE;
	if (pCntx->callback)
	{
		pCntx->callback((const WCHAR *)pCntx->filepath,
			(const WCHAR *)pCntx->fileNamePostionIn_filepath);
	}

	threedo_mfree((void *)pCntx);
}

S32 threedo_audio_play(const WCHAR *path, void (*callback)(const WCHAR *abs_path, const WCHAR *fname))
{
	struct sric_context *sricContext;
	MDI_RESULT result;

	if (mdi_audio_is_recording() || mdi_audio_is_playing(MDI_AUDIO_PLAY_ANY))
		return TD_ERRNO_MODULE_BUSING;

	sricContext = (struct sric_context *)threedo_malloc(sizeof(struct sric_context));
	memset(sricContext, 0, sizeof(struct sric_context));
	app_ucs2_strcpy((S8*)sricContext->filepath, (const S8 *)path);
	
	result = mdi_audio_play_file((void *)path, 
		DEVICE_AUDIO_PLAY_ONCE,
		NULL, 
		_td_audio_play_callback,
		(void *)sricContext);

	{
		WCHAR buf[128];

		memset(buf, 0, sizeof(buf));
		kal_wsprintf(buf, "audio play");
		mmi_popup_display_simple(buf, MMI_EVENT_SUCCESS, GRP_ID_ROOT, NULL);
	}

	TD_TRACE("[media] audio play start(result=%d, cb=0x%0x).", result, callback);
	if (MDI_AUDIO_SUCCESS == result)
    {
		sricContext->callback = callback;
		sricContext->status = SRIC_STATE_PLAYING;
		return TD_ERRNO_NO_ERROR;
	}
	
	threedo_mfree((void *)sricContext);
    return TD_ERRNO_ADOPLY_FAILED;
}

S32 threedo_audio_play_stop(void)
{
	mdi_audio_stop_all();

	return TD_ERRNO_NO_ERROR;
}

static U8 _td_audio_play_respond(void *p)
{
	threedo_ado_play_req *req = (threedo_ado_play_req*)p;

	threedo_audio_play((const WCHAR*)req->path, req->callback);
	return 1;
}

void threedo_audio_play_request(WCHAR *path)
{
	threedo_ado_play_req *req = (threedo_ado_play_req*)threedo_construct_para(sizeof(threedo_ado_play_req));

	app_ucs2_strcpy((S8*)req->path, (const S8*)path);
	
	threedo_send_message_to_mmi(MSG_ID_THREEDO_FILE_PLAY_REQ, (void*)req);
}

void threedo_media_initialize(void)
{
	threedo_set_message_protocol(MSG_ID_THREEDO_FILE_PLAY_REQ, _td_audio_play_respond);
}

const char *threedo_media_get_state_string(void)
{
	if (mdi_audio_is_recording())
		return "record";

	if (mdi_audio_is_playing(MDI_AUDIO_PLAY_ANY))
		return "play";
	
	return "idle";
}

#if defined(__PROJECT_FOR_ZHANGHU__)
void threedo_custom_audio_record_stop(void)
{
	if (IsMyTimerExist(THREEDO_MEDIA_AUDIO_RECORD_LIMITED_TIMER))
	{
		StopTimer(THREEDO_MEDIA_AUDIO_RECORD_LIMITED_TIMER);
	}

	threedo_audio_record_stop();
}

static void _td_custom_audio_record_callback(const WCHAR *abs_path, const WCHAR *fname)
{
	//调用socket进行发送
	U32 fileSize;
		
    //检查文件大小并调用发送数据的函数
	fileSize = _td_audio_record_check_file(abs_path, TD_TRUE);
	if (fileSize >= 200 && threedo_get_usable_sim_index() >= 0)
	{
		//将文件名写入文件保存起来
		//send file by socket.
		threedo_send_file_trans_req((const WCHAR *)abs_path,
			fileSize,
		#if defined(THREEDO_ADO_QUALITY_LEVEL) && (THREEDO_ADO_QUALITY_LEVEL == 1)
			TD_TT_TYPE_WAV,
		#else
			TD_TT_TYPE_AMR,
		#endif
			TD_TT_ACTION_IS_UPLOAD);		
	}
}

S32 threedo_custom_audio_record_start(void)
{
	if (TD_ERRNO_NO_ERROR == threedo_audio_record(THREEDO_ADO_LIMITED_SEC*1000, 30,
				threedo_get_usable_drive(),
				_td_custom_audio_record_callback))
	{
		StartTimer(THREEDO_MEDIA_AUDIO_RECORD_LIMITED_TIMER,
			18000, threedo_custom_audio_record_stop);
		
		return TD_ERRNO_NO_ERROR;
	}

	return TD_ERRNO_ADOREC_START_FILED;
}

#else

S32 threedo_custom_audio_record_start(void)
{
	return TD_ERRNO_NO_ERROR;
}
#endif

