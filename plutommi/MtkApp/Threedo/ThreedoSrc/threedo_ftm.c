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
#include "kal_public_api.h"

#include "fs_gprot.h"
#include "app_str.h"

#include "mmi_rp_app_threedo_def.h"

#include "ThreedoGprot.h"
#include "threedo_errno.h"
#include "threedo_socket.h"
#include "threedo_ftm.h"
#if defined(__PROJECT_FOR_ZHANGHU__)
#include "threedo_media.h"
#endif
#include "threedo_parse.h"


#if !defined(__THREEDO_HTTP_KEEP_ALIVE__)
#define __THREEDO_HTTP_KEEP_ALIVE__
#endif

//#if defined(__PROJECT_FOR_ZHANGHU__)
//#define __THREEDO_DOWNLOAD_AFTER_UPLOAD__
//#endif

/*
http://192.168.1.200:23000/myServer/?uid=-1&fid=310&tpn=mp3
模块向服务器发出上传请求：
-----------------------------------------------------------------
POST /myServer/?uid=-1&fid=310&tpn=mp3 HTTP/1.1<CR><LF>
Content-Length: 3462<CR><LF>
Connection: Keep-Alive<CR><LF>
Content-Type: binary/octet-stream<CR><LF>
Host: 192.168.1.200:23000<CR><LF>
<CR><LF>
[upload file data]
-----------------------------------------------------------------
这里就是模块通过TCP连接向服务器发送文件数据，直到发完为止。
-----------------------------------------------------------------
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
-----------------------------------------------------------------
模块向服务器发出下载请求：
-----------------------------------------------------------------
GET /myServer/?uid=-1&fid=310&tpn=mp3 HTTP/1.1<CR><LF>
Connection: Keep-Alive<CR><LF>
Content-Type: binary/octet-stream<CR><LF>
Host: 192.168.1.200:23000<CR><LF>
<CR><LF>
-----------------------------------------------------------------
接着就是模块通过TCP连接接收服务器发下来的文件数据，直到完成。
-----------------------------------------------------------------
*/
static void _td_fttm_get_adapt_apn(char *apn);

static U32 _td_fttm_get_http_download_head(char *buffer, const char *hostString, const char *param, U16 port);

static U32 _td_fttm_get_http_upload_head(char *buffer, const char *hostString, const char *param ,U16 port, U32 size);

static void _td_file_trans_finish_notify(U8 action, U8 type, U32 size, const S8* fpath);

static void _td_file_trans_upload_continue(void);

static void _td_file_trans_read_socket(void);

static void _td_file_trans_write_socket(void);

static void _td_file_trans_check_and_clear_task(void);

static void _td_file_trans_after_connect(void);

static U8 _td_file_trans_event_cb(void *evt);

static U8 _td_file_trans_host_name_cb(void *evt);

static U8 _td_file_trans_respond(void *param);

static threedo_fttm *tdFttmCntx = NULL;

#define TD_FTM_SOC_TIMEOUT_ID    THREEDO_TIMER_KEEP_ACTIVE
#define TD_FTM_SOC_HEARTBEAT_ID  THREEDO_TIMER_SOCKET_RECONN

static void _sh_fttm_init_check(void)
{	
	if (NULL == tdFttmCntx)
	{
		tdFttmCntx = (threedo_fttm*)threedo_malloc(sizeof(threedo_fttm));
		TD_ASSERT(NULL != tdFttmCntx);
		memset(tdFttmCntx, 0, sizeof(threedo_fttm));
		tdFttmCntx->sock.soc_id = THREEDO_INVALID_SOCID;
		threedo_read_config_info(&tdFttmCntx->conf);
		memset(tdFttmCntx->conf.apn, 0, sizeof(tdFttmCntx->conf.apn));
		_td_fttm_get_adapt_apn(tdFttmCntx->conf.apn);

		kal_wsprintf((WCHAR*)tdFttmCntx->buffer, "%s\\%s", THREEDO_FOLDER, tdFttmCntx->conf.dlsavefd);
		threedo_create_path_multi(threedo_get_usable_drive(), (const WCHAR *)tdFttmCntx->buffer);
	}
}

//申请一个节点存储空间
static threedo_tt_node *_sh_fttm_node_alloc(S32 action)
{
	if (action)
	{
		int i;
		
		for (i=0; i<TD_FTTM_TASK_MAX; i++)
		{
			if (TD_TT_ACTION_IS_UNKOWN == tdFttmCntx->tQueue[i].action)
			{
				memset(&tdFttmCntx->tQueue[i], 0, sizeof(threedo_tt_node));
				tdFttmCntx->tQueue[i].action = action;
				tdFttmCntx->tQueue[i].insertTick = TD_GET_TICK();
				return &tdFttmCntx->tQueue[i];
			}
		}
	}
	TD_TRACE("[fttm] alloc node failed.");
	return NULL;
}

//释放一个节点存储空间
static void _sh_fttm_node_free(threedo_tt_node *node)
{
	TD_TRACE("[fttm] free node , action=%d.", node->action);
	memset(node, 0, sizeof(threedo_tt_node));
}

//在任务链的尾巴上追加一个任务
S32 _sh_fttm_task_append(threedo_tt_node *node)
{
	S32 result = TD_FALSE;
	threedo_tt_node **root;

	if (tdFttmCntx)
	{
		root = &tdFttmCntx->task;
		while (NULL != *root)
		{
			root = &((*root)->next);
		}
		node->state = TD_TT_WAIT_START;
		*root = node;
		
		result = TD_TRUE;
	}

	TD_TRACE("[fttm] append node result=%d, action=%d.", result, node->action);
	
	return result;
}

//从任务链中移除一个节点
static S32 _sh_fttm_task_remove(threedo_tt_node *node) 
{
	if (tdFttmCntx)
	{
		threedo_tt_node **root = &tdFttmCntx->task;
		
		while (NULL != *root)
		{
			if ((*root)->insertTick == node->insertTick)
			{
				TD_TRACE("[fttm] remove node, action=%d.", node->action);
				*root = node->next;
				break;
			}

			root = &(*root)->next;
		}
		
		_sh_fttm_node_free(node);
	}

	return TD_FALSE;
}

static S32 _sh_fttm_tasks_check_domain_and_port(void)
{
	threedo_tt_node *curr, *next;
	
	if (NULL == tdFttmCntx->task)
		return 0;

	if (NULL == tdFttmCntx->task->next)
		return 0;

	curr = tdFttmCntx->task;
	next = tdFttmCntx->task->next;

	if (0 == app_stricmp((char*)curr->domain, (char*)next->domain) &&
		curr->port == next->port)
	{
		return 1;
	}

	return 0;
}

//将节点移动到链表尾部
static S32 _sh_fttm_task_move_to_tail(threedo_tt_node *node)
{
	if (tdFttmCntx)
	{
		threedo_tt_node **root = &tdFttmCntx->task;
		
		while (NULL != *root)
		{
			if ((*root)->insertTick == node->insertTick)
			{
				*root = node->next;	
				node->next = NULL;
				TD_TRACE("[fttm] move node to tail.");
				_sh_fttm_task_append(node);
				return TD_TRUE;
			}

			root = &(*root)->next;
		}
	}

	TD_TRACE("[fttm] move node to tail failed.");
	return TD_FALSE;
}



/* download
   TD_TT_WAIT_START
      TD_TT_ENTER_PROC     --> write GET 
         TD_TT_SENT_HEAD   --> read data
            TD_TT_TRANS_CONTI --> read data
               TD_TT_TRANS_ENDED

	upload
    TD_TT_WAIT_START
       TD_TT_ENTER_PROC     --> write POST 
          TD_TT_SENT_HEAD   --> write data
             TD_TT_TRANS_CONTI --> write data
                TD_TT_TRANS_ENDED
*/


//GET /files/Ado5513b1820.amr HTTP/1.1
//Host: f2.yunba.io:8888
//Connection: keep-alive
//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
//User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.101 Safari/537.36
///DNT: 1
//Referer: http://f2.yunba.io:8888/files/
//Accept-Encoding: gzip, deflate, sdch
//Accept-Language: zh-CN,zh;q=0.8,en;q=0.6


//HTTP/1.1 200 OK
//Date: Thu, 26 Mar 2015 13:01:21 GMT
//X-Powered-By: Express
//Accept-Ranges: bytes
//Cache-Control: public, max-age=0
//Last-Modified: Thu, 26 Mar 2015 07:15:47 GMT
//ETag: W/"13e6-3829749211"
//Content-Type: application/octet-stream
//Content-Length: 5094
//X-Via: 1.1 ccsskan113017:8101 (Cdn Cache Server V2.0)
//Connection: keep-alive

#if defined(__PROJECT_FOR_ZHANGHU__)
#if defined(__THREEDO_DOWNLOAD_AFTER_UPLOAD__)
static void _td_fttm_test_download(WCHAR *fpath, U32 fsize, U8 ftype)
{
	//test code. download after upload finished.
	TD_TRACE("[fttm] Send download request. TEST CODE.");
	memset(tdFttmCntx->buffer, 0, TD_FTTM_SOC_BUF_SIZE);
	kal_wsprintf((WCHAR*)tdFttmCntx->buffer,
		"%c:\\%s\\%s\\",
		threedo_get_usable_drive(),
		THREEDO_FOLDER,
		tdFttmCntx->conf.dlsavefd);
	if (!threedo_create_path_multi_ex((const WCHAR *)tdFttmCntx->buffer))
	{
		TD_TRACE("[fttm] create download path failed.");
		return;
	}
	app_ucs2_strcat((char*)tdFttmCntx->buffer,
		(const char*)threedo_get_file_name_from_path(fpath));

	threedo_send_file_trans_req(
		(const WCHAR *)tdFttmCntx->buffer, 
		fsize,
		ftype,
		TD_TT_ACTION_IS_DOWNLOAD);
}
#endif


void threedo_download_start(WCHAR *filename)
{
	U8 temp[512];
	memset(temp, 0, 512);
	kal_wsprintf((WCHAR*)temp,
		"%c:\\%s\\%s\\",
		threedo_get_usable_drive(),
		THREEDO_FOLDER,
		tdFttmCntx->conf.dlsavefd);
	if (!threedo_create_path_multi_ex((const WCHAR *)temp))
	{
		TD_TRACE("[fttm] create download path failed.");
		return;
	}
	app_ucs2_strcat((char*)temp,
		(const char*)(filename));

	TD_TRACE("threedo_download_start");
	threedo_send_file_trans_req(
		(const WCHAR *)temp, 
		0,
		0,
		TD_TT_ACTION_IS_DOWNLOAD);

}

static U32 _td_fttm_get_http_upload_head_zhanghu(
	char *head,
	char *tail,
	const char *host,
	const char *param,
	const char *fname,
	const char *dataMd5,
	U16 port,
	U32 size,
	U32 dataCrc32,
	U32 transId)
{
	char boundary[64];
	U32 templength;
	char *contentHead;

	memset(boundary, 0, 64);
	kal_sprintf(boundary, "%s%08x%08x", http_form_boundary, dataCrc32, transId);
	
	templength = kal_sprintf(tail,
			"\r\n--%s\r\n"
			"Content-Disposition: form-data; name=\"signature\"\r\n"
			"\r\n"
			"%s\r\n"
			"--%s--\r\n",
			boundary,
			dataMd5,
			boundary);
	
	TD_TRACE("[fttm] generete upload tail, length=%d.", templength);

	size += templength;
	contentHead = (char*)head+512;
	templength = kal_sprintf(contentHead,
			"--%s\r\n"
			"Content-Disposition: form-data; name=\"path\"\r\n"
			"\r\n"
			"%s\r\n"
			"--%s\r\n"
			"Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n"
			"Content-Type: application/octet-stream\r\n"
			"\r\n",
			boundary,
			fname,
			boundary,
			fname);
	TD_TRACE("[fttm] generete upload tail0, length=%d.", templength);
	
	templength = kal_sprintf(head,
			"POST %s %s\r\n"
			"Host: %s:%d\r\n"
		#if defined(__THREEDO_HTTP_KEEP_ALIVE__)
			"Connection: keep-alive\r\n"
		#endif
			"Content-Length: %d\r\n"
			"Content-Type: multipart/form-data; boundary=%s\r\n"
			"\r\n%s",
			param, http_version,
			host, port,
			size + templength,
			boundary,
			contentHead);


	return templength;
}
#endif

static void _td_fttm_stop_block_timer(void)
{
	if (IsMyTimerExist(TD_FTM_SOC_TIMEOUT_ID))
	{
		TD_TRACE("[fttm] stop timer");
		StopTimer(TD_FTM_SOC_TIMEOUT_ID);
	}
}

static void _td_fttm_block_timer_cb(void)
{
	threedo_ft_timeout_ind *ind;
	
	TD_TRACE("[fttm] timer experiod.");
	ind = (threedo_ft_timeout_ind*)threedo_construct_para(sizeof(threedo_ft_timeout_ind));
	ind->timer_id = TD_FTM_SOC_TIMEOUT_ID;
	threedo_send_message(
		MSG_ID_THREEDO_TIMER_EXPERIOD_IND,
		(void*)ind,
		kal_get_active_module_id(),
		MOD_THREEDO,
		0);
	
}

static void _td_fttm_start_block_timer(U32 ms)
{
	TD_TRACE("[fttm] start timer, delay %d ms.", ms);
	StartTimer(TD_FTM_SOC_TIMEOUT_ID,
			ms,
			_td_fttm_block_timer_cb);
}

static void _td_fttm_heartbeat_timer_cb(void)
{
	threedo_ft_timeout_ind *ind;
	
	TD_TRACE("[fttm] timer experiod--heartbeat.");
	ind = (threedo_ft_timeout_ind*)threedo_construct_para(sizeof(threedo_ft_timeout_ind));
	ind->timer_id = TD_FTM_SOC_HEARTBEAT_ID;
	threedo_send_message(
		MSG_ID_THREEDO_TIMER_EXPERIOD_IND,
		(void*)ind,
		kal_get_active_module_id(),
		MOD_THREEDO,
		0);
}

static void _td_fttm_stop_heartbeat_timer(void)
{
	if (IsMyTimerExist(TD_FTM_SOC_HEARTBEAT_ID))
	{
		TD_TRACE("[fttm] stop heartbeat timer--heartbeat.");
		StopTimer(TD_FTM_SOC_HEARTBEAT_ID);
	}
}

static void _td_fttm_start_heartbeat_timer(void)
{
	if (NULL == tdFttmCntx)
		return;
	
	TD_TRACE("[fttm] start heartbeat timer.");
	StartTimer(TD_FTM_SOC_HEARTBEAT_ID,
			tdFttmCntx->conf.heartbeat*1000,
			_td_fttm_heartbeat_timer_cb);
}

static void _td_fttm_get_adapt_apn(char *apn)
{
	char plmn[8];
	S32 simid;

	memset(plmn, 0, 8);
	if (0 <= (simid = threedo_get_usable_sim_index()))
	{
	#if defined(__MTK_TARGET__)
		if (srv_sim_ctrl_get_home_plmn((mmi_sim_enum)(0x01<<simid), plmn, 7))
		{
			if (0 == strncmp("46001", plmn, 5))
				strcpy(apn, "uninet");
			else
				strcpy(apn, "cmnet");
		}
		else
		{
			strcpy(apn, "internet");
		}
	#else
		strcpy(apn, "internet");
	#endif
	}

	TD_TRACE("[fttm] SIM%d, plmn \"%s\", apn \"%s\".", simid+1, plmn, apn);
}

static U32 _td_fttm_get_http_download_head(char *buffer, const char *hostString, const char *param, U16 port)
{
	return kal_sprintf(buffer,
			"GET %s %s\r\n"
			"Host: %s:%d\r\n"
		#if defined(__THREEDO_HTTP_KEEP_ALIVE__)
			"Connection: Keep-Alive\r\n"
			"Keep-Alive: timeout=100\r\n"
		#endif
			"Accept: */*\r\n"
			"\r\n",
			param, http_version,
			hostString, port);
}

static U32 _td_fttm_get_http_upload_head(char *buffer, const char *hostString, const char *param ,U16 port, U32 size)
{
	return kal_sprintf(buffer,
			"POST %s %s\r\n"
			"Host: %s:%d\r\n"
			"Content-Type: binary/octet-stream\r\n"
		#if defined(__THREEDO_HTTP_KEEP_ALIVE__)
			"Connection: Keep-Alive\r\n"
			"Keep-Alive: timeout=100\r\n"
		#endif
			"Content-Length: %d\r\n"
			"\r\n",
			param, http_version,
			hostString, port,
			size);
}

static U32 _td_fttm_get_http_heartbeat_head(char *buffer, const char *hostString, const char *param ,U16 port, U32 size)
{
	return kal_sprintf(buffer,
			"POST %s\r\n"
			"Host: %s:%d\r\n"
			"Connection: Keep-Alive\r\n"
			"\r\n",
			http_version,
			hostString, port);
}

static void _td_file_trans_finish_notify(U8 action, U8 type, U32 size, const S8* fpath)
{
	threedo_ft_finish_ind *ind;
	
	ind = (threedo_ft_finish_ind*)threedo_construct_para(sizeof(threedo_ft_finish_ind));
	ind->error  = 0;
	ind->action = action;
	ind->size   = size;
	ind->type   = type;
	app_ucs2_strcpy((S8*)ind->path, (const S8*)fpath);
	threedo_send_message(
		MSG_ID_THREEDO_FILE_TRANS_FINISH_IND,
		(void *)ind,
		kal_get_active_module_id(),
		MOD_THREEDO,
		0);
}

static void _td_file_trans_upload_continue(void)
{
	U32 read;
	U32 send;
	
	tdFttmCntx->task->state = TD_TT_UPLOAD_CONTI;
	if (tdFttmCntx->task->fsHandle <= FS_NO_ERROR)
	{
		tdFttmCntx->task->fsHandle = FS_Open((const WCHAR*)tdFttmCntx->task->path, FS_READ_ONLY);
		tdFttmCntx->task->filePoint = 0;
		tdFttmCntx->task->currSize  = 0;
		if (tdFttmCntx->task->fsHandle <= FS_NO_ERROR)
		{
			//打开文件失败
			TD_TRACE("[fttm] upload continue, open local file failed(%d).", tdFttmCntx->task->fsHandle);
			_td_file_trans_check_and_clear_task();
			return;
		}
	}

	TD_TRACE("[fttm] upload continue process.");
	do {
		FS_Read(tdFttmCntx->task->fsHandle,
			(void*)tdFttmCntx->buffer,
			TD_FTTM_SOC_BUF_SIZE,
			&read);

		if (read > 0)
		{
			tdFttmCntx->task->filePoint += read;
			
			send = (U32)threedo_socket_write(&tdFttmCntx->sock,
							(void *)tdFttmCntx->buffer,
							read);
			if (tdFttmCntx->sock.error > 0)
			{
				tdFttmCntx->task->currSize += send;
				TD_TRACE("[fttm] upload continue, readed=%d, send=%d, currSize=%d. filesize=%d.", read, send, tdFttmCntx->task->currSize, tdFttmCntx->task->fileSize);
				if (send != read)
				{
					tdFttmCntx->task->filePoint -= (read - send);
					FS_Seek(tdFttmCntx->task->fsHandle, tdFttmCntx->task->filePoint, FS_FILE_BEGIN);
				}
			}
			else if (tdFttmCntx->sock.error == SOC_WOULDBLOCK)
			{
				TD_TRACE("[fttm] upload continue, blocked.");
				
				tdFttmCntx->task->filePoint -= read;
				FS_Seek(tdFttmCntx->task->fsHandle, tdFttmCntx->task->filePoint, FS_FILE_BEGIN);
				_td_fttm_start_block_timer(TD_FTTM_SOC_BLOCK_TIME);
				break;
			}
			else
			{
				//数据丢失了? 关闭socket
				TD_TRACE("[fttm] upload continue, data package not send end.");	
				_td_file_trans_check_and_clear_task();
				break;
			}
		}
		else if (read != 0)
		{
			//read nothings.
			_td_file_trans_check_and_clear_task();
			break;
		}
		
		if (tdFttmCntx->task->currSize == tdFttmCntx->task->fileSize)
		{
			//文件上传完毕
			tdFttmCntx->task->retryCt = 0;
			if (tdFttmCntx->task->fsHandle > FS_NO_ERROR)
			{
				FS_Close(tdFttmCntx->task->fsHandle);
			}
			tdFttmCntx->task->fsHandle = 0;
			
		#if defined(__THREEDO_HTTP_KEEP_ALIVE__)
			tdFttmCntx->last_upload_read = TD_TRUE;
		#endif
		
		#if defined(__PROJECT_FOR_ZHANGHU__)
			send = (U32)threedo_socket_write(&tdFttmCntx->sock,
					(void *)tdFttmCntx->tail,
					strlen(tdFttmCntx->tail));
			TD_TRACE("[fttm] upload send MD5 string & upload tail(%dBytes).", send);
			
			TD_TRACE("[fttm] upload finish, total=%d.sock:%i", 
				tdFttmCntx->task->currSize, tdFttmCntx->sock.error);
			if (tdFttmCntx->sock.error > 0)
			{
				tdFttmCntx->task->state = TD_TT_FD_TRANS_EOF;
				//启动定时器，延时2-3秒钟关闭
				_td_fttm_start_block_timer(1000);
			}
			
			else if (tdFttmCntx->sock.error == SOC_WOULDBLOCK)
			{
				tdFttmCntx->task->state = TD_TT_UPLOAD_TAIL;
				_td_fttm_start_block_timer(TD_FTTM_SOC_BLOCK_TIME);
			}
			
			else
			{
				//threedo_socket_close(&tdFttmCntx->sock);
				_td_file_trans_check_and_clear_task();
			}
		#else
			tdFttmCntx->task->state = TD_TT_FD_TRANS_EOF;
			_td_fttm_start_block_timer(3000);
		#endif
		
			break;
		}
		else  if (0 == read)
		{
			TD_TRACE("[fttm] upload continue, read return 0, currsize=%d, filesize=%d,filepoint=%d.",
				tdFttmCntx->task->currSize, tdFttmCntx->task->fileSize, tdFttmCntx->task->filePoint);
			_td_file_trans_check_and_clear_task();
			break;
		}

	}while(tdFttmCntx->sock.error > 0);

}

static void _td_file_trans_download_continue(void)
{
	U32 read;
	U32 wrote;
	U32 bufsize = TD_FTTM_SOC_BUF_SIZE;
#if defined(__PROJECT_FOR_ZHANGHU__)
	bufsize += TD_FTTM_HTTP_TAIL_BUF_SIZE;
#endif

	tdFttmCntx->task->state = TD_TT_DOWNLOAD_CONTI;
	if (tdFttmCntx->task->fsHandle <= FS_NO_ERROR)
	{
		tdFttmCntx->task->filePoint = 0;
		tdFttmCntx->task->currSize = 0;
		tdFttmCntx->task->fsHandle = FS_Open((const WCHAR*)tdFttmCntx->task->path, FS_READ_WRITE|FS_CREATE_ALWAYS);
		if (tdFttmCntx->task->fsHandle <= FS_NO_ERROR)
		{
			//打开文件失败
			TD_TRACE("[fttm] upload continue, open local file failed(%d).", tdFttmCntx->task->fsHandle);
			_td_file_trans_check_and_clear_task();
			return;
		}
	}

	TD_TRACE("[fttm] download continue process.");
	do {
		read = (U32)threedo_socket_read(&tdFttmCntx->sock, (void *)tdFttmCntx->buffer, bufsize);
		if (tdFttmCntx->sock.error > 0)
		{
			FS_Seek(tdFttmCntx->task->fsHandle, tdFttmCntx->task->filePoint, FS_FILE_BEGIN);
			FS_Write(tdFttmCntx->task->fsHandle, (void*)tdFttmCntx->buffer, read, &wrote);
			tdFttmCntx->task->filePoint += wrote;
			tdFttmCntx->task->currSize += read;

			TD_TRACE("[fttm] download, save. filepoint=%d, currsize=%d,filesize=%d.",
				tdFttmCntx->task->filePoint, tdFttmCntx->task->currSize, tdFttmCntx->task->fileSize);
			if (tdFttmCntx->task->currSize == tdFttmCntx->task->fileSize)
			{
				//文件上传完毕
				TD_TRACE("[fttm] download finish, total=%dBytes.", tdFttmCntx->task->currSize);
				tdFttmCntx->task->state = TD_TT_FD_TRANS_EOF;
				FS_Commit(tdFttmCntx->task->fsHandle);
				FS_Close(tdFttmCntx->task->fsHandle);
				tdFttmCntx->task->fsHandle = 0;
				//启动定时器，延时1-2秒钟关闭
				_td_fttm_start_block_timer(1000);
				break;
			}
		}
		else if (tdFttmCntx->sock.error == SOC_WOULDBLOCK)
		{
			TD_TRACE("[fttm] download blocked.");
			_td_fttm_start_block_timer(/*TD_FTTM_SOC_BLOCK_TIME*/3000);			
			break;
		}
		else
		{
			_td_file_trans_check_and_clear_task();
			break;
		}
	}while(tdFttmCntx->sock.error > 0);
}

static void _td_file_trans_read_socket()
{
	if (tdFttmCntx->sock.soc_id < 0)
		return;

//	TD_TRACE("[fttm] read socket process, task[action=%d, state=%d], last_upload_read=%d.",
//		tdFttmCntx->task->action, tdFttmCntx->task->state, tdFttmCntx->last_upload_read);
	
	switch (tdFttmCntx->task->state)
	{
	case TD_TT_FD_TRANS_EOF:
		if (TD_TT_ACTION_IS_UPLOAD == tdFttmCntx->task->action ||
			TD_TT_ACTION_IS_HEARTBEAT == tdFttmCntx->task->action)
		{
			HTTP_RSP_HEAD head;
			
			tdFttmCntx->length = (U32)threedo_socket_read(
									&tdFttmCntx->sock,
									(void*)tdFttmCntx->buffer,
									TD_FTTM_SOC_BUF_SIZE);
			
		#if defined(WIN32)
			tdFttmCntx->buffer[tdFttmCntx->length] = 0;
			TD_TRACE("[fttm] ==> upload EOF state, read %d Bytes.", tdFttmCntx->length);
			TD_TRACE("%s", tdFttmCntx->buffer);
			TD_TRACE("[fttm] <==");
		#endif

			if (tdFttmCntx->sock.error > 0)
			{				
				threedo_socket_parse_http_head(&head, (char*)tdFttmCntx->buffer, tdFttmCntx->length);

				if (THREEDO_HTTP_RETVAL_OK != head.errorCode)
				{
					TD_TRACE("[fttm] upload finish, server return error=%d.", head.errorCode);
					tdFttmCntx->task->fileSize = tdFttmCntx->task->currSize = 0;
				}
			}
		#if defined(__THREEDO_HTTP_KEEP_ALIVE__)
			tdFttmCntx->last_upload_read = TD_FALSE;
		#endif
			tdFttmCntx->task->state = TD_TT_TRANS_FINISH;
		}
		else if (TD_TT_ACTION_IS_DOWNLOAD == tdFttmCntx->task->action)
		{
			//既然文件都发送完毕了还没收到服务器的回应, 
			//我们本地还是认为文件已经上传完成，所以移除这个任务
		#if defined(WIN32)
			if (NULL != strstr(tdFttmCntx->buffer, http_version))
			{
				TD_TRACE("%s", tdFttmCntx->buffer);
			}
		#endif
		}
		_td_file_trans_check_and_clear_task();			
		break;
		
	case TD_TT_HTTP_GET_SENT:
		if (TD_TT_ACTION_IS_DOWNLOAD == tdFttmCntx->task->action)
		{
			//发送完GET请求后，现在收到的应该是服务器的回应包
			//如果没有错，那么回应的码值应该为200，即"HTTP/1.1 200 OK"
			U32 bufsize = TD_FTTM_SOC_BUF_SIZE;
		#if defined(__PROJECT_FOR_ZHANGHU__)
			bufsize += TD_FTTM_HTTP_TAIL_BUF_SIZE;
		#endif
			
			tdFttmCntx->length = (U32)threedo_socket_read(&tdFttmCntx->sock, (void *)tdFttmCntx->buffer, bufsize);
			if (tdFttmCntx->sock.error > 0)
			{
				//解析HTTP头并保存数据
				HTTP_RSP_HEAD head;
				U32 wrote;
				char *position = (char*)tdFttmCntx->buffer;
				U32   length = tdFttmCntx->length;
			
			#if defined(__THREEDO_HTTP_KEEP_ALIVE__)
				// HTTP/1.1 200 OK
				// Date: Wed, 01 Apr 2015 21:35:25 GMT
				// X-Powered-By: Express
				// Content-Type: application/json; charset=utf-8
				// Content-Length: 27
				// ETag: W/"1b-7d759a54"
				// X-Via: 1.1 ccsskan113017:8101 (Cdn Cache Server V2.0)
				// Connection: keep-alive
				// 
				// {"path":"Ado551c9bf70.amr"}
				// GET /files/Ado551c9bf70.amr HTTP/1.1
				// Host: f2.yunba.io:8888
				// Connection: Keep-Alive
				// Accept: */*
				// 
				// 
				// HTTP/1.1 200 OK
				// Date: Wed, 01 Apr 2015 21:35:26 GMT
				// X-Powered-By: Express
				// Accept-Ranges: bytes
				// Cache-Control: public, max-age=0
				// Last-Modified: Thu, 02 Apr 2015 01:31:20 GMT
				// ETag: W/"13e6-3225829100"
				// Content-Type: application/octet-stream
				// Content-Length: 5094
				
				#if defined(__PROJECT_FOR_ZHANGHU__)
				if (tdFttmCntx->last_upload_read)
				{
					threedo_socket_parse_http_head(&head, (const char *)position, length);
					if (THREEDO_HTTP_RETVAL_OK == head.errorCode &&
						THREEDO_HTTTP_CONTENT_TYPE_IS_JSON == head.content_type &&
						0 < head.contentLength)
					{
						if (NULL != strstr((char*)head.content, (const char*)"{\"path\":"))
						{
							//find this tag, ignore.
							char *ret;
							if (NULL != (ret = strstr((char*)head.content, (const char*)http_version)))
							{
								length = length - (U32)(ret-position);
								position = ret;
							}
							else
							{
								length = length - head.contentLength - ((char*)head.content - position);
								position = (char*)head.content + head.contentLength;
							}

							TD_TRACE("[fttm] include last ul response, content-length=%d. left-length=%d", head.contentLength, length);
							tdFttmCntx->last_upload_read = TD_FALSE;
							if (0 == length)
							{
								break;
							}
						}
					}
				}
				#endif
				tdFttmCntx->last_upload_read = TD_FALSE;
			#endif
			
				threedo_socket_parse_http_head(&head, (const char *)position, length);
				TD_TRACE("[fttm] recieved firtst download response, error=%d, length=%d.", head.errorCode, head.contentLength);
				if (THREEDO_HTTP_RETVAL_OK == head.errorCode)
				{
					//保存文件
					if (tdFttmCntx->task->fsHandle <= FS_NO_ERROR)
					{
					#if defined(WIN32)
						TD_TRACE("[fttm] download, save path: %s", tdFttmCntx->task->path);
					#endif
						TD_TRACE("<<download save path: %s>>", tdFttmCntx->task->path);
						tdFttmCntx->task->fsHandle = FS_Open((const WCHAR*)tdFttmCntx->task->path, FS_READ_WRITE|FS_CREATE_ALWAYS);
						if (tdFttmCntx->task->fsHandle > FS_NO_ERROR)
						{
							tdFttmCntx->task->currSize = 0;
							tdFttmCntx->task->filePoint = 0;
							tdFttmCntx->task->fileSize = (U32)head.contentLength;
							if (FS_Extend(tdFttmCntx->task->fsHandle, tdFttmCntx->task->fileSize) < FS_NO_ERROR)
							{
								_td_file_trans_check_and_clear_task();
								break;
							}
							else
							{
								FS_Seek(tdFttmCntx->task->fsHandle, tdFttmCntx->task->fileSize, FS_FILE_BEGIN);
								FS_Truncate(tdFttmCntx->task->fsHandle);
								FS_Seek(tdFttmCntx->task->fsHandle, 0, FS_FILE_BEGIN);
							}
						}
					}

					//save
					FS_Write(tdFttmCntx->task->fsHandle, (void*)head.content, tdFttmCntx->length - (U32)head.headLength, &wrote);
					tdFttmCntx->task->filePoint += wrote;
					tdFttmCntx->task->currSize += (tdFttmCntx->length - (U32)head.headLength);
					_td_file_trans_download_continue();
				}
				else if (THREEDO_HTTP_RETVAL_NOT_FOUND == head.errorCode) 
				{
					TD_TRACE("[fttm] file not found.");
					_td_fttm_stop_block_timer();
					_sh_fttm_task_remove(tdFttmCntx->task);
				}
				else
				{
					TD_TRACE("[fttm] parse http head failed.");
					_td_file_trans_check_and_clear_task();
				}
			}
			else if (tdFttmCntx->sock.error == SOC_WOULDBLOCK)
			{
				_td_fttm_start_block_timer(TD_FTTM_SOC_BLOCK_TIME);
			}
			else
			{
				_td_file_trans_check_and_clear_task();
			}
		}
		break;

	case TD_TT_DOWNLOAD_CONTI:
		if (TD_TT_ACTION_IS_DOWNLOAD == tdFttmCntx->task->action)
		{
			_td_file_trans_download_continue();
		}
		break;

	default:
		TD_TRACE("[fttm] read socket, task state is error(%d).", tdFttmCntx->task->state);
		break;
	}
}

static void _td_file_trans_write_socket(void)
{
	U32 temp;
	S32 heartbeat;
	
	if (tdFttmCntx->sock.soc_id < 0)
		return;

	TD_TRACE("[fttm] write socket process, task[action=%d, state=%d].", tdFttmCntx->task->action, tdFttmCntx->task->state);
	heartbeat = (TD_TT_ACTION_IS_HEARTBEAT == tdFttmCntx->task->action) ? 1 : 0;
	switch (tdFttmCntx->task->state)
	{
	case TD_TT_READY_SOCKET:
		tdFttmCntx->length = 0;
		if (TD_TT_ACTION_IS_UPLOAD == tdFttmCntx->task->action || heartbeat)
		{
		#if defined(__PROJECT_FOR_ZHANGHU__)
			{
				char name[32];
				char md5[36];

				memset(tdFttmCntx->buffer, 0, TD_FTTM_SOC_BUF_SIZE);

				memset(name, 0, 32);
				app_ucs2_str_to_asc_str(name, (S8*)threedo_get_file_name_from_path((WCHAR *)tdFttmCntx->task->path));
				
				memset(md5, 0, 36);
				threedo_md5_file(md5, (const WCHAR*)tdFttmCntx->task->path);
				
				tdFttmCntx->length = _td_fttm_get_http_upload_head_zhanghu(
										(char*)tdFttmCntx->buffer,
										(char*)tdFttmCntx->tail,
										(const char*)tdFttmCntx->task->domain,
										(const char*)tdFttmCntx->task->urlParam,
										(const char *)name,
										(const char *)md5,
										tdFttmCntx->task->port,
										tdFttmCntx->task->fileSize,
										threedo_crc32_file((const WCHAR*)tdFttmCntx->task->path),
										threedo_get_system_tick());
			}
		#else
		
			tdFttmCntx->length = _td_fttm_get_http_upload_head(
								(char*)tdFttmCntx->buffer,
								(const char*)tdFttmCntx->task->domain,
								(const char*)tdFttmCntx->task->urlParam,
								tdFttmCntx->task->port,
								tdFttmCntx->task->fileSize);
		#endif
		
		#if defined(WIN32)
			TD_TRACE("[fttm] ==> upload head length:%d Bytes", tdFttmCntx->length);
			TD_TRACE("%s", tdFttmCntx->buffer);
			TD_TRACE("[fttm] <==");
		#endif
		}
		else if (TD_TT_ACTION_IS_DOWNLOAD == tdFttmCntx->task->action)
		{
			tdFttmCntx->length = _td_fttm_get_http_download_head(
								(char*)tdFttmCntx->buffer,
								(const char*)tdFttmCntx->task->domain,
								(const char*)tdFttmCntx->task->urlParam,
								tdFttmCntx->task->port);

		#if defined(WIN32)
			TD_TRACE("[fttm] ==> download request head length:%d Bytes", tdFttmCntx->length);
			TD_TRACE("%s", tdFttmCntx->buffer);
			TD_TRACE("[fttm] <==");
		#endif
		}
		else if (TD_TT_ACTION_IS_HEARTBEAT == tdFttmCntx->task->action)
		{
			tdFttmCntx->length = _td_fttm_get_http_heartbeat_head(
								(char*)tdFttmCntx->buffer,
								(const char*)tdFttmCntx->task->domain,
								(const char*)tdFttmCntx->task->urlParam,
								tdFttmCntx->task->port,
								0);
		#if defined(WIN32)
			TD_TRACE("[fttm] ==> hearbeat head length:%d Bytes", tdFttmCntx->length);
			TD_TRACE("%s", tdFttmCntx->buffer);
			TD_TRACE("[fttm] <==");
		#endif
		}

		if (tdFttmCntx->length)
		{
			temp = (U32)threedo_socket_write(&tdFttmCntx->sock, 
								(void *)tdFttmCntx->buffer,
								tdFttmCntx->length);
			if ((S32)tdFttmCntx->sock.error > 0 /*(S32)tdFttmCntx->length*/)
			{
				TD_TRACE("[fttm] task action:%d request have sent %d Bytes.", tdFttmCntx->task->action, temp);
				if (TD_TT_ACTION_IS_UPLOAD == tdFttmCntx->task->action || heartbeat)
				{
					tdFttmCntx->task->state = TD_TT_HTTP_POST_SENT;
					_td_file_trans_upload_continue();
				}
				else if (TD_TT_ACTION_IS_DOWNLOAD == tdFttmCntx->task->action)
				{
					tdFttmCntx->task->state = TD_TT_HTTP_GET_SENT;
				}
			}
			else if (tdFttmCntx->sock.error == SOC_WOULDBLOCK)
			{
				TD_TRACE("[fttm] download request have sent, blocked...");
				_td_fttm_start_block_timer(TD_FTTM_SOC_BLOCK_TIME);
			}
			else
			{
				TD_TRACE("[fttm] download request have sent, happened some errors.");
				_td_file_trans_check_and_clear_task();
			}
		}
		break;
		
	case TD_TT_UPLOAD_CONTI:
		TD_TRACE("[fttm] here is upload continue state. actioin=%d.", tdFttmCntx->task->action);
		if (TD_TT_ACTION_IS_UPLOAD == tdFttmCntx->task->action || heartbeat)
		{
			_td_file_trans_upload_continue();
		}
		break;

	case TD_TT_UPLOAD_TAIL:
		#if defined(__PROJECT_FOR_ZHANGHU__)
			{
				U32 send;
				send = (U32)threedo_socket_write(&tdFttmCntx->sock,
						(void *)tdFttmCntx->tail,
						strlen(tdFttmCntx->tail));
				TD_TRACE("[fttm] upload file tail(%dBytes).", send);
				
				if (tdFttmCntx->sock.error > 0)
				{
					tdFttmCntx->task->state = TD_TT_FD_TRANS_EOF;
					//启动定时器，延时2-3秒钟关闭
					_td_fttm_start_block_timer(2000);
				}
				else if (tdFttmCntx->sock.error == SOC_WOULDBLOCK)
				{
					_td_fttm_start_block_timer(TD_FTTM_SOC_BLOCK_TIME);
				}
				else
				{
					//threedo_socket_close(&tdFttmCntx->sock);
					tdFttmCntx->task->retryCt ++;
					_td_file_trans_check_and_clear_task();
				}
			}
		#endif
		break;
		
	case TD_TT_FD_TRANS_EOF:
		TD_TRACE("[fttm] here is trans ended state. actioin=%d.", tdFttmCntx->task->action);
		/*
		//这里应该是关闭socket的操作了
		if (TD_TT_ACTION_IS_UPLOAD == tdFttmCntx->task->action || heartbeat)
		{
			//既然文件都发送完毕了还没收到服务器的回应, 
			//我们本地还是认为文件已经上传完成，所以移除这个任务
			_sh_fttm_task_remove(tdFttmCntx->task);
			threedo_socket_close(tdFttmCntx->sock);
		}
		*/
		_td_file_trans_check_and_clear_task();
		break;
		
	default:
		TD_TRACE("[fttm] write socket, task state is error(%d).", tdFttmCntx->task->state);
		break;
	}
}

static void _td_file_trans_check_and_clear_task(void)
{
#if defined(__THREEDO_HTTP_KEEP_ALIVE__)
	S32 keep_http_alive = 0;
#endif

	if (NULL == tdFttmCntx)
		return;
	
	_td_fttm_stop_block_timer();
	_td_fttm_stop_heartbeat_timer();
	
	if (NULL != tdFttmCntx->task)
	{
		if (tdFttmCntx->task->fsHandle > FS_NO_ERROR)
		{
			FS_Close(tdFttmCntx->task->fsHandle);
			tdFttmCntx->task->fsHandle = 0;
		}

		TD_TRACE("[fttm] CACT, retryCt=%d,currSize=%d,fileSize=%d,state=%d,action=%d.", 
			tdFttmCntx->task->retryCt,
			tdFttmCntx->task->currSize,
			tdFttmCntx->task->fileSize,
			tdFttmCntx->task->state,
			tdFttmCntx->task->action);

		switch (tdFttmCntx->task->state)
		{
		case TD_TT_TRANS_FINISH:
			{
				_td_file_trans_finish_notify(
					tdFttmCntx->task->action,
					tdFttmCntx->task->type,
					tdFttmCntx->task->fileSize,
					(const S8 *)tdFttmCntx->task->path);
				
			#if defined(__THREEDO_DOWNLOAD_AFTER_UPLOAD__)
				_td_fttm_test_download(
					(WCHAR *)tdFttmCntx->task->path,
					tdFttmCntx->task->fileSize,
					tdFttmCntx->task->type);
			#endif
			
				_sh_fttm_task_remove(tdFttmCntx->task);
			}
			break;
			
		case TD_TT_FD_TRANS_EOF:			
			//准备结束上传任务, 但是它的状态又不是先读取一次socket看看有不有数据
			if (TD_TT_ACTION_IS_UPLOAD == tdFttmCntx->task->action ||
				TD_TT_ACTION_IS_HEARTBEAT == tdFttmCntx->task->action)
			{
				U32 read;

				if (tdFttmCntx->sock.soc_id >= 0)
				{
				#if defined(__PROJECT_FOR_ZHANGHU__)
					read = threedo_socket_read(&tdFttmCntx->sock, (void *)tdFttmCntx->buffer, TD_FTTM_SOC_BUF_SIZE+TD_FTTM_HTTP_TAIL_BUF_SIZE);
				#else
					read = threedo_socket_read(&tdFttmCntx->sock, (void *)tdFttmCntx->buffer, TD_FTTM_SOC_BUF_SIZE)
				#endif

					tdFttmCntx->last_upload_read = TD_FALSE;
					if (tdFttmCntx->sock.error > 0)
					{
						HTTP_RSP_HEAD head;
						threedo_socket_parse_http_head(&head, (const char *)tdFttmCntx->buffer, read);
						tdFttmCntx->last_upload_read = TD_FALSE;
					#if defined(__THREEDO_HTTP_KEEP_ALIVE__)
						if (THREEDO_HTTP_NOT_FOUND_TAG !=  head.keep_alive)
							keep_http_alive = head.keep_alive;
						else
							keep_http_alive = 0;
					#endif
					
						TD_TRACE("[fttm] CACT, upload finish. server return result:%d,length:%d.", head.errorCode, head.contentLength);
						if (THREEDO_HTTP_RETVAL_OK == head.errorCode)
						{
							_td_file_trans_finish_notify(
								tdFttmCntx->task->action,
								tdFttmCntx->task->type,
								tdFttmCntx->task->fileSize,
								(const S8 *)tdFttmCntx->task->path);
						}
					}
					else if (tdFttmCntx->sock.error == SOC_WOULDBLOCK)
					{
						threedo_tt_node *next = tdFttmCntx->task->next;

						tdFttmCntx->last_upload_read = TD_TRUE;
						if (tdFttmCntx->task->retryCt >= TD_FTTM_RETRY_MAX)
						{
							//等了这么久没有回应，应该是丢包了，TCP丢包了，是否要关闭掉socket呢?
							//threedo_socket_close(&tdFttmCntx->sock);
							TD_TRACE("[fttm] CACT, upload finish. recive resepond have try %d times. stop", tdFttmCntx->task->retryCt);
						}
						else if (NULL != next && TD_TT_ACTION_IS_DOWNLOAD == next->action)
						{
							TD_TRACE("[fttm] CACT, upload finish. recive resepond in next download task.");
						}
						else
						{
							//waiting???
							tdFttmCntx->task->retryCt ++;
							_td_fttm_start_block_timer(1000);
							return;
						}
					}
					else
					{
						threedo_socket_close(&tdFttmCntx->sock);
					}
				}
			#if defined(__THREEDO_DOWNLOAD_AFTER_UPLOAD__)
				_td_fttm_test_download(
					(WCHAR *)tdFttmCntx->task->path,
					tdFttmCntx->task->fileSize,
					tdFttmCntx->task->type);
			#endif
				_sh_fttm_task_remove(tdFttmCntx->task);
			}
			else if (TD_TT_ACTION_IS_DOWNLOAD == tdFttmCntx->task->action)
			{
			#if defined(__PROJECT_FOR_ZHANGHU__)
				TD_TRACE("[fttm] CACT, download EOF, send play request.");
				threedo_audio_play_request((WCHAR*)tdFttmCntx->task->path);
			#endif
				_sh_fttm_task_remove(tdFttmCntx->task);
				//download finish processed. return here.
			}
			break;
			
		default:
			{
				if (TD_FTTM_RETRY_MAX < tdFttmCntx->task->retryCt+1)
				{
					TD_TRACE("[fttm] CACT, remove task because retry too more times.");
					_sh_fttm_task_remove(tdFttmCntx->task);
				}
				else
				{
					tdFttmCntx->task->state = TD_TT_WAIT_START;
					tdFttmCntx->task->filePoint = 0;
					tdFttmCntx->task->currSize = 0;
				#if defined(__THREEDO_HTTP_KEEP_ALIVE__)
					keep_http_alive = _sh_fttm_tasks_check_domain_and_port();
				#endif
					
					if (NULL != tdFttmCntx->task)
					{
						if (TD_TT_ACTION_IS_HEARTBEAT == tdFttmCntx->task->action)
							_sh_fttm_task_remove(tdFttmCntx->task);
						else if (NULL != tdFttmCntx->task->next)
							_sh_fttm_task_move_to_tail(tdFttmCntx->task);
					}
				}
			}
			break;
		}		
	}
	else
	{
	#if defined(__THREEDO_HTTP_KEEP_ALIVE__)
		keep_http_alive = tdFttmCntx->sock.soc_id >= 0 ? 1 : 0;
	#endif
	}
	
	tdFttmCntx->ticks = TD_GET_TICK();
	if (tdFttmCntx->sock.soc_id >= 0)
	{
		if (tdFttmCntx->sock.error < SOC_SUCCESS && tdFttmCntx->sock.error != SOC_WOULDBLOCK)
		{
			tdFttmCntx->task->currSize = tdFttmCntx->task->filePoint = 0;
			tdFttmCntx->task->state = TD_TT_WAIT_START;
			threedo_socket_close(&tdFttmCntx->sock);

		#if defined(__THREEDO_HTTP_KEEP_ALIVE__)
			keep_http_alive = 0;
		#endif
		}
		
	#if !defined(__THREEDO_HTTP_KEEP_ALIVE__)
		threedo_socket_close(&tdFttmCntx->sock);
	#endif
	}
	
	if (NULL != tdFttmCntx->task)
	{
		threedo_file_trans_process();
	}
	else
	{
		//keep heartbeat.
		_td_fttm_start_heartbeat_timer();
	}
}

static void _td_file_trans_send_heartbeat_pre(void)
{
	threedo_tt_node **root = &tdFttmCntx->task;
	threedo_tt_node *node;
	
	while (*root != NULL)
	{
		node = *root;
		if (TD_TT_ACTION_IS_HEARTBEAT == node->action)
		{
			*root = node->next;
			_sh_fttm_node_free(node);
		}
	}
}

static void _td_file_trans_send_heartbeat(void)
{
	threedo_tt_node *node;
	
	if (NULL == tdFttmCntx)
		return;

	if (NULL == tdFttmCntx->task)
	{
		_td_file_trans_send_heartbeat_pre();
		node = _sh_fttm_node_alloc(TD_TT_ACTION_IS_HEARTBEAT);
		if (NULL != node)
		{
			S32 fd;
			
			node->type     = TD_TT_TYPE_HRTBT;
		#if defined(__PROJECT_FOR_ZHANGHU__)
			kal_wsprintf((WCHAR*)node->path, "%c:\\%s\\heart.beat", threedo_get_usable_drive(), THREEDO_FOLDER);
		#else
			kal_wsprintf((WCHAR*)node->path, "%c:\\%s\\heart.beat", threedo_get_usable_drive(), THREEDO_FOLDER);
		#endif

			if (FS_NO_ERROR < (fd = FS_Open((const WCHAR*)node->path, FS_READ_WRITE|FS_CREATE_ALWAYS)))
			{
				char buffer[16];
				memset(buffer, 0, 16);
				node->fileSize = kal_sprintf(buffer, "UTC:%d", threedo_get_current_time());
				FS_Write(fd, (void*)buffer, node->fileSize, &node->fileSize);
				FS_Commit(fd);
				FS_Close(fd);
			}
			
		#if defined(__PROJECT_FOR_ZHANGHU__)
			strcpy(node->domain, tdFttmCntx->conf.uldomain);
			node->port = tdFttmCntx->conf.ulport;
		#endif
			
			sprintf(node->urlParam, "%s", tdFttmCntx->conf.ulrpath);
			_sh_fttm_task_append(node);
		}
	}
	
	threedo_file_trans_process();
}

static void _td_file_trans_after_connect(void)
{	
	if (tdFttmCntx->sock.soc_id < 0)
		return;
	
	switch (tdFttmCntx->sock.error)
	{
	case SOC_SUCCESS:
		//开始收发数据
		_td_file_trans_write_socket();
		break;
		
	case SOC_WOULDBLOCK:
		//设置超时等待定时器
		_td_fttm_start_block_timer(TD_FTTM_SOC_BLOCK_TIME);
		break;
		
	default:
		//关闭处理, 重连等操作
		_td_file_trans_check_and_clear_task();
		break;
	}
}


static U8 _td_file_trans_event_cb(void *evt)
{
	app_soc_notify_ind_struct *note = (app_soc_notify_ind_struct*)evt;


	if (tdFttmCntx && tdFttmCntx->sock.soc_id != note->socket_id)
		return TD_FALSE;
	
	//如果定时器还在跑, 要停止先
	_td_fttm_stop_block_timer();
	_td_fttm_stop_heartbeat_timer();
	
	switch (note->event_type)
	{
	case SOC_READ:
		_td_file_trans_read_socket();
		break;
		
	case SOC_CONNECT:
		if (note->result)
		{
			_td_file_trans_write_socket();
		}
		else
		{
			_td_file_trans_check_and_clear_task();
		}
		break;
		
	case SOC_WRITE:
		_td_file_trans_write_socket();
		break;

	case SOC_CLOSE:
		//关闭socket
		if (!note->result && SOC_CONNRESET == note->error_cause)
		{
			//对方关闭了socket连接
			TD_TRACE("[fttm] fttm socket closed by remote.");
			threedo_socket_close(&tdFttmCntx->sock);
		}
		
		_td_file_trans_check_and_clear_task();
		break;
		
	default:
		break;
	}
	
	return TD_TRUE;
}

static U8 _td_file_trans_host_name_cb(void *evt)
{
	app_soc_get_host_by_name_ind_struct *note = (app_soc_get_host_by_name_ind_struct*)evt;

	if (NULL == tdFttmCntx)
		return TD_FALSE;
	
	TD_TRACE("[fttm] DNS responed, rst=%d,entry=%d,state=0x%x,req(%d,%d).",
		note->result, note->num_entry,tdFttmCntx->sock.get_ip,
		tdFttmCntx->sock.req_id, note->request_id);
	
	if (tdFttmCntx->sock.req_id != note->request_id &&
		tdFttmCntx->sock.get_ip != THREEDO_DNS_START)
		return TD_FALSE;

	_td_fttm_stop_block_timer();
	_td_fttm_stop_heartbeat_timer();
	tdFttmCntx->sock.error = SOC_SUCCESS;
	
	if (note->result)
	{
		tdFttmCntx->sock.get_ip = THREEDO_DNS_IGNORE;	
	#if defined(__THREEDO_USE_LAST_DNS_RESULT__)
		memset(&tdFttmCntx->dns, 0, sizeof(trheedo_dns_ret));
		tdFttmCntx->dns.result = note->result;
		strcpy(tdFttmCntx->dns.domain, tdFttmCntx->task->domain);
		tdFttmCntx->dns.addr_len = note->addr_len;
		memcpy(tdFttmCntx->dns.addr, note->addr, sizeof(tdFttmCntx->dns.addr));
		memcpy(tdFttmCntx->dns.entry, note->entry, sizeof(tdFttmCntx->dns.entry));
		tdFttmCntx->dns.num_entry = note->num_entry;
		tdFttmCntx->dns.ticks = TD_GET_TICK();
	#endif
	
		tdFttmCntx->sock.addr.addr_len = note->addr_len;
		memset(tdFttmCntx->sock.addr.addr, 0 , sizeof(tdFttmCntx->sock.addr.addr));
		memcpy(tdFttmCntx->sock.addr.addr, note->addr, note->addr_len);
		threedo_socket_connect(&tdFttmCntx->sock);
		_td_file_trans_after_connect();
	}
	else
	{
		//域名解析失败, 延时后重来
		//_td_file_trans_check_and_clear_task();
		tdFttmCntx->sock.get_ip = THREEDO_DNS_FAILED;	
		_td_fttm_start_block_timer(TD_FTTM_SOC_BLOCK_TIME);
	}
	
	return TD_TRUE;
}

void threedo_file_trans_process_ready(threedo_ft_proc_req *arg)
{
	threedo_tt_node *node;

	if (NULL == arg)
		return;

	//读取任务队列的第一个任务，
	//根据任务的属性执行动作
	if (NULL == tdFttmCntx)
	{
		_sh_fttm_init_check();
	}

	if (NULL != (node = _sh_fttm_node_alloc(arg->action)))
	{
		node->type     = arg->type;
		node->fileSize = arg->size;
		app_ucs2_strcpy((S8*)node->path, (const S8*)arg->path);
		
	#if defined(__PROJECT_FOR_ZHANGHU__)
		if (TD_TT_ACTION_IS_UPLOAD == arg->action)
		{
			strcpy(node->domain, tdFttmCntx->conf.uldomain);
			node->port = tdFttmCntx->conf.ulport;
			sprintf(node->urlParam, "%s", tdFttmCntx->conf.ulrpath);//"/upload"
		}
		else
		{
			TD_TRACE("<<<<download, %s>>>>", (S8*)(arg->path));
			strcpy(node->domain, tdFttmCntx->conf.dldomain);
			node->port = tdFttmCntx->conf.dlport;
			sprintf(node->urlParam, "%s/", tdFttmCntx->conf.dlrpath);//"/files/"
			app_ucs2_str_to_asc_str(
				(S8*)(node->urlParam+strlen(node->urlParam)), 
				(S8*)threedo_find_file_name_position(arg->path));
				//(S8*)(arg->path));
		}
	#endif
	
		_sh_fttm_task_append(node);
	}

}

void threedo_file_trans_process(void)
{
	threedo_tt_node *task;
	U32 ticks = TD_GET_TICK();
	static int retry = 0;
	
	if (NULL == tdFttmCntx->task)
	{
		TD_TRACE("[fttm] fttm, initialized error.");
		return;
	}

do_process:
	task = tdFttmCntx->task;
	TD_TRACE("[fttm] fttm, state=%d,action=%d,iTick=%d,cTick=%d.",
		task->state, task->action, task->insertTick, ticks);
	
	if (TD_TT_WAIT_START != task->state)
	{
		retry++;
		if (retry > 1) {
			_td_fttm_stop_block_timer();
			_sh_fttm_task_remove(task);
			TD_TRACE("[fttm] fttm, kk remove task bacause retry times limited.");
			if (NULL != tdFttmCntx->task)
			{
				goto do_process;
			}
		}
		TD_TRACE("[fttm] fttm, the first task is processing(action=%d,state=%d)...",task->action, task->state);
		return;
	}
	retry = 0;
	

	if (TD_FTTM_RETRY_MAX < ++task->retryCt)
	{
		_sh_fttm_task_remove(task);
		TD_TRACE("[fttm] fttm, remove task bacause retry times limited.");
		if (NULL != tdFttmCntx->task)
		{
			goto do_process;
		}
	}
	
	if ((ticks - task->insertTick > TD_FTTM_TAKE_TICK_MAX) &&
		NULL != task->next)
	{
		_sh_fttm_task_remove(task);
		TD_TRACE("[fttm] fttm, remove task bacause task timeout.");
		if (NULL != tdFttmCntx->task)
		{
			goto do_process;
		}
		return;
	}
	
	if (TD_TT_ACTION_IS_DOWNLOAD == task->action ||
		TD_TT_ACTION_IS_UPLOAD == task->action ||
		TD_TT_ACTION_IS_HEARTBEAT == task->action)
	{	
		if (0 == tdFttmCntx->app_id)
		{
			threedo_socket_allocate_app_id(&tdFttmCntx->app_id);
		}
		if (0 == tdFttmCntx->acc_id)
		{
			tdFttmCntx->acc_id = threedo_socket_get_account_id(
									tdFttmCntx->conf.apn,
									tdFttmCntx->app_id,
									TD_SOC_TYPE_SIM1_TCP);

		}

		if (tdFttmCntx->sock.soc_id < 0)
		{
			S32 do_dns = 1;

		#if defined(__THREEDO_USE_LAST_DNS_RESULT__)
			//之前有做DNS域名解析，且保留的结果是一致的
			if (ticks - tdFttmCntx->dns.ticks > (1000*100*32))
			{
				memset(&tdFttmCntx->dns, 0, sizeof(trheedo_dns_ret));
			}
			
			if (tdFttmCntx->dns.result &&
				0 == app_stricmp((char*)tdFttmCntx->dns.domain, (char*)task->domain))
			{
				//use last result
				char ip[16];
				soc_dns_a_struct *adr = &tdFttmCntx->dns.entry[0];
				
				memset(ip, 0, 16);
				sprintf(ip, "%d.%d.%d.%d", adr[0], adr[1], adr[2], adr[3]);
				TD_TRACE("[fttm] task proc, last DNS ip=\"%s\".", ip);
				
				threedo_socket_open(
					&tdFttmCntx->sock,
					ip,
				#if defined(__THREEDO_SOCKET_IN_MMI_MOD__)
					MOD_MMI,
				#else
					MOD_THREEDO,
				#endif
					tdFttmCntx->acc_id,
					0,
					task->port);
				
				TD_TRACE("[fttm] task proc, use last DNS,soc:%d,err:%d.",
					tdFttmCntx->sock.soc_id, tdFttmCntx->sock.error);
				
				if (tdFttmCntx->sock.soc_id >= 0)
				{
					do_dns = 0;
					if (tdFttmCntx->sock.get_ip == THREEDO_DNS_IGNORE)
					{
						//可以直接使用
						tdFttmCntx->dns.result = 0;
					}
				}
				else
				{
					threedo_socket_close(&tdFttmCntx->sock);
				}
			}
		#else
			do_dns = 1;
		#endif
		
			if (do_dns)
			{
				threedo_socket_open(
					&tdFttmCntx->sock,
					task->domain,
				#if defined(__THREEDO_SOCKET_IN_MMI_MOD__)
					MOD_MMI,
				#else
					MOD_THREEDO,
				#endif
					tdFttmCntx->acc_id,
					0,
					task->port);
			}
		}
		else
		{
			TD_TRACE("[fttm] process task, socket is valid.");
			tdFttmCntx->sock.error = SOC_SUCCESS;
		}

		if (tdFttmCntx->sock.soc_id >= 0)
		{
			task->state = TD_TT_READY_SOCKET;
			TD_TRACE("[fttm] process task, sock.get_ip=0x%x.axtion=%d", tdFttmCntx->sock.get_ip, task->action);
			if (tdFttmCntx->sock.get_ip == THREEDO_DNS_IGNORE)
			{
				//ok, 直接搞起
				_td_file_trans_after_connect();
			}
			else
			{
				switch (tdFttmCntx->sock.get_ip)
				{
				case THREEDO_DNS_SUCCESS:
					_td_file_trans_after_connect();
					break;
				case THREEDO_DNS_BLOCKED:
					//域名解析中, 设置超时等待定时器
					_td_fttm_start_block_timer(TD_FTTM_SOC_BLOCK_TIME);
					break;
				default:
					threedo_socket_close(&tdFttmCntx->sock);
					_td_fttm_start_block_timer(TD_FTTM_SOC_BLOCK_TIME);
					break;
				}
			}
		}
		else
		{
			TD_TRACE("[fttm] fttm, create socket failed. try again later.");
			_td_fttm_start_block_timer(TD_FTTM_SOC_BLOCK_TIME);
		}
	}
	else
	{
		TD_TRACE("[fttm] fttm, action(%d) is not defined.", task->action);
	}
}

void threedo_send_file_trans_req(const WCHAR *path, U32 size, U8 type, U8 act)
{
	threedo_ft_proc_req *req = (threedo_ft_proc_req*)threedo_construct_para(sizeof(threedo_ft_proc_req));

	app_ucs2_strcpy((S8*)req->path, (const S8*)path);
	req->size   = size;
	req->type   = type;
	req->action = act;
	
	threedo_send_message(MSG_ID_THREEDO_FILE_TRANS_REQ,
		(void*)req,
		kal_get_active_module_id(),
		MOD_THREEDO,
		0);
}

void threedo_file_trans_timeout_proc(void *v)
{
	threedo_ft_timeout_ind *ind = (threedo_ft_timeout_ind*)v;

	if (TD_FTM_SOC_TIMEOUT_ID == ind->timer_id)
	{
		_td_file_trans_check_and_clear_task();
	}
	else if (TD_FTM_SOC_HEARTBEAT_ID == ind->timer_id)
	{
		_td_file_trans_send_heartbeat();
	}
}

void threedo_file_trans_init(void)
{
#if defined(__THREEDO_SOCKET_IN_MMI_MOD__)
	threedo_set_message_protocol_multi(
		MSG_ID_APP_SOC_GET_HOST_BY_NAME_IND,
		_td_file_trans_host_name_cb);

	threedo_set_message_protocol_multi(
		MSG_ID_APP_SOC_NOTIFY_IND, 
		_td_file_trans_event_cb);
#endif
}

void threedo_file_trans_free_all(void)
{
	if (tdFttmCntx)
	{
		if (tdFttmCntx->sock.soc_id >= 0)
		{
			threedo_socket_close(&tdFttmCntx->sock);
		}

		threedo_socket_free_app_id(tdFttmCntx->app_id);

		if (tdFttmCntx->task)
		{
			if (tdFttmCntx->task->fsHandle > FS_NO_ERROR)
			{
				FS_Commit(tdFttmCntx->task->fsHandle);
				FS_Close(tdFttmCntx->task->fsHandle);
				tdFttmCntx->task->fsHandle = 0;
			}

			while(NULL != tdFttmCntx->task)
			{
				_sh_fttm_node_free(tdFttmCntx->task);
			}
		}

		_td_fttm_stop_block_timer();
		_td_fttm_stop_heartbeat_timer();
		
		threedo_mfree((void *)tdFttmCntx);
		tdFttmCntx = NULL;
	}
}

S32 threedo_file_trans_socket_event_handle(ilm_struct *ilm)
{
	S32 proced = 1;
	
#if defined(__THREEDO_SOCKET_IN_MMI_MOD__)
	return 0;
#endif

	switch(ilm->msg_id)
	{
	case MSG_ID_APP_SOC_GET_HOST_BY_NAME_IND:
		_td_file_trans_host_name_cb((void *)ilm->local_para_ptr);
		break;
	case MSG_ID_APP_SOC_NOTIFY_IND:
		_td_file_trans_event_cb((void *)ilm->local_para_ptr);
		break;
	default:
		proced = 0;
		break;
	}

	return proced;
}

const char *threedo_file_trans_get_state_string(void)
{
	return "";	
}

void threedo_file_trans_check_task_queue(void)
{
	//如果一个任务的总时间超过了一定时长则干掉它
	if (tdFttmCntx)
	{
		threedo_tt_node **root = &tdFttmCntx->task;
		threedo_tt_node *node;
		U32 tick = TD_GET_TICK();
		
		while (NULL != *root)
		{
			if ((*root)->insertTick - tick >= TD_FTTM_TAKE_TICK_MAX)
			{
				node = (*root)->next;
				_sh_fttm_node_free(*root);
				TD_TRACE("[fttm] remove node because take time too long.");
			}

			root = &node;
		}
	}
}


#if defined(__PROJECT_FOR_ZHANGHU__)
//5分钟发一个心跳包
void threedo_keep_active(void)
{
	//StartTimer(THREEDO_TIMER_KEEP_ACTIVE, 5*60*1000, threedo_keep_active);
}
#endif


