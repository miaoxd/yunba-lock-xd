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

//#include "soc_api.h"
//#include "soc_consts.h"
//#include "cbm_consts.h"
//#include "DtcntSrvIntStruct.h"
//#include "DtcntSrvDb.h"
//#include "app2soc_struct.h"
#include "mmi_rp_app_threedo_def.h"

#if defined(__THREEDO_TEST_CODE__)
#include "fs_gprot.h"
#endif

#include "ThreedoGprot.h"
#include "threedo_errno.h"
#include "threedo_socket.h"

const char *http_head_line_end  = "\r\n";
const char *http_head_body_split= "\r\n\r\n";
const char *http_version        = "HTTP/1.1";
const char *http_content_length = "Content-Length";
const char *http_connection     = "Connection";

const char *http_content_type   = "Content-Type";
const char *http_content_dispositon = "Content-Disposition";
const char *http_form_boundary  = "----WebKitFormBoundary";
const char *http_form_data      = "form-data";


S32 threedo_socket_allocate_app_id(U8 *idout)
{
	S32 ret = cbm_register_app_id(idout);
	TD_TRACE("[sock] socket allocate app id return %d, error=%d.", *idout, ret);
	return (S32)(CBM_OK == ret);
}

void threedo_socket_free_app_id(U8 id)
{
	cbm_deregister_app_id(id);
}

U32 threedo_socket_get_account_id(char *apn, U8 appid, U32 flags)
{
	U32 accid = 0;
	
	if (0xF0 == (flags & 0xF0))
	{
		//只针对WIFI连接
		accid = cbm_encode_data_account_id(CBM_WIFI_ACCT_ID, CBM_SIM_ID_SIM1, appid, TD_FALSE);
		TD_TRACE("[sock] socket encode data account id(WIFI) return 0x%x.",accid);
	}
	else
	{
		if (NULL == apn || 0 == strlen(apn))
			return 0;
		
		if (TD_FALSE)
		{
			srv_dtcnt_result_enum ret;
			ret = srv_dtcnt_get_acc_id_by_apn(apn, &accid);
			TD_TRACE("[sock] socket get account id by \"%s\" rerturn 0x%x.", apn, ret);
		}
		else
		{
			cbm_sim_id_enum cbmsim = CBM_SIM_ID_SIM1;
	    		srv_dtcnt_account_info_st *accinfo = NULL;
			accinfo = srv_dtcnt_db_store_get_acc_info_by_apn(apn);
			if (NULL != accinfo)
			{
				cbmsim = srv_dtcnt_db_store_cvt_dtcnt_sim_to_cbm_sim(accinfo->sim_info);
				accid = cbm_encode_data_account_id((U32)accinfo->acc_id, cbmsim, appid, TD_FALSE);
				TD_TRACE("[sock] socket get account id return 0x%x.", accid);
			}
			else
			{
				TD_TRACE("[sock] socket get account info by \"%s\" failed.", apn);
			}
		}
	}

	return accid;
}

S32 threedo_socket_open(
	THRD_SOC_MNGR *pSoc,
	char *addr,
	S32 mod, //MOD_MMI
	U32 accid,
	S32 type,//socket_type_enum
	U16 port)
{
	soc_error_enum error = SOC_SUCCESS;
	U8 option;
	U8 ip_addr[4];
	kal_bool ip_validity;


	if (NULL == pSoc || 0 == port ||
		NULL == addr || 0 == strlen(addr))
	{
		TD_TRACE("[sock] socket open, illegal parameter.");
		return TD_ERRNO_PARAM_ILLEGAL;
	}

	memset(pSoc, 0, sizeof(THRD_SOC_MNGR));
	pSoc->soc_id = THREEDO_INVALID_SOCID;
	
	if (soc_ip_check(addr, ip_addr, &ip_validity))
	{
		//OK, IP string.
		TD_TRACE("threedo ip string, %s", addr);
		pSoc->addr.addr_len = 4;
		memcpy(pSoc->addr.addr, ip_addr, pSoc->addr.addr_len);
	}
	else
	{
		TD_TRACE("threedo dns string, %s", addr);
		//OK, host name.
		pSoc->get_ip = THREEDO_DNS_START;
	}

	pSoc->addr.port      = port;
	pSoc->addr.sock_type = (socket_type_enum)type;
	pSoc->req_id         = (S32)TD_GET_TICK();
	
	//创建
#if defined(__THREEDO_SOCKET_IN_MMI_MOD__)
	mod = MOD_MMI;
#endif

	pSoc->soc_id = soc_create(SOC_PF_INET, pSoc->addr.sock_type, 0, (module_type)mod, accid);
	TD_TRACE("[sock] socket create return %d, sock_type=%d, reqid=0x%x",
		pSoc->soc_id, pSoc->addr.sock_type, pSoc->req_id);
    if (pSoc->soc_id < 0)
    {
		return TD_ERRNO_SOC_CREATE_FAIL;
	}

	//异步
	option = SOC_READ | SOC_WRITE | SOC_CLOSE;
	if (SOC_SOCK_STREAM == pSoc->addr.sock_type)
	{
		option |= SOC_CONNECT;
	}
	error = (soc_error_enum)soc_setsockopt(pSoc->soc_id, SOC_ASYNC, &option, sizeof(option));
	if (SOC_SUCCESS != error)
	{
		TD_TRACE("[sock] socket set ASYNC option result=%d. soc=%d.", error, pSoc->soc_id);
		soc_close(pSoc->soc_id);
		return TD_ERRNO_SOC_SET_ASYNC;
	}

	//非阻塞
	option = TD_TRUE;
	error = (soc_error_enum)soc_setsockopt(pSoc->soc_id, SOC_NBIO, &option, sizeof(option));
	if (SOC_SUCCESS != error)
	{
		TD_TRACE("[sock] socket set NBIO option result=%d. soc=%d.", error, pSoc->soc_id);
		soc_close(pSoc->soc_id);
		return TD_ERRNO_SOC_SET_NBIO;
	}

	if (THREEDO_DNS_START == pSoc->get_ip)
	{
		U8 adrl;
		pSoc->error = (soc_error_enum)soc_gethostbyname(TD_FALSE, 
							(module_type)mod, 
							pSoc->req_id,
							(const char*)addr,
							pSoc->addr.addr,
							&adrl,
							0,
							accid);
		TD_TRACE("[sock] socket get host \"%s\", (state:0x%x) error=%d.", addr, pSoc->get_ip, pSoc->error);		
		switch(pSoc->error)
		{
		case SOC_SUCCESS:
			pSoc->get_ip = THREEDO_DNS_IGNORE;
			pSoc->addr.addr_len = (S16)adrl;
			break;
			
		case SOC_WOULDBLOCK:
			pSoc->get_ip = THREEDO_DNS_BLOCKED;			
			return TD_ERRNO_NO_ERROR;
			
		default:
			pSoc->get_ip = THREEDO_DNS_IGNORE;
			return TD_ERRNO_SOCKET_FAILED;
		}
	}

	//连接
	return threedo_socket_connect(pSoc);
}

void threedo_socket_close(THRD_SOC_MNGR *pSoc)
{
	S32 ret;
	
	if (NULL == pSoc || pSoc->soc_id < 0)
	{
		TD_TRACE("[sock] socket close, illegal parameters.");
		return;
	}
	
	if (THREEDO_DNS_IGNORE != pSoc->get_ip)
	{
		soc_abort_dns_query(0, 0, 1, (U32)pSoc, 0, 0, 0, 0);
	}
	
	ret = soc_close(pSoc->soc_id);
	TD_TRACE("[sock] socket(soc:%d,req:0x%x) closed(%d).", pSoc->soc_id, pSoc->req_id, ret);
	memset(pSoc, 0, sizeof(THRD_SOC_MNGR));
	pSoc->soc_id = THREEDO_INVALID_SOCID;
}

S32 threedo_socket_connect(THRD_SOC_MNGR *pSoc)
{
	if (NULL == pSoc)
	{
		TD_TRACE("[sock] socket connect, illegal parameters.");
		return TD_ERRNO_PARAM_ILLEGAL;
	}

	if (THREEDO_DNS_IGNORE != pSoc->get_ip)
	{
		TD_TRACE("[sock] socket connect, split host name processing.");
		return TD_ERRNO_SOCKET_SPLITING;
	}

	//连接
	if (SOC_SOCK_STREAM == pSoc->addr.sock_type)
	{
		//TCP
		//这里怎么会出现问题呢? S8 类型的result 与 -2 比较出了问题, 奇怪
		pSoc->error = (soc_error_enum)soc_connect(pSoc->soc_id, &pSoc->addr);
		switch(pSoc->error)
		{
		case SOC_WOULDBLOCK:
			TD_TRACE("[sock] socket tcp connect blocked.");
			break;
		case SOC_SUCCESS:
			/* do nothing. */
			TD_TRACE("[sock] socket tcp connect successful.");
			break;
			
		default:
			{
				S8 err;
				S32 detail;
				
				soc_get_last_error(pSoc->soc_id, &err, &detail);
				TD_TRACE("[sock] socket tcp connect failed, error=%d, detail=%d.", err, detail);
			}
			return TD_ERRNO_UNKOWN_ERR;
		}
	}
	else
	{
		//UDP
		//http://blog.csdn.net/butterflydog/article/details/7375057
		//UDP可以直接多次调用connect
		pSoc->error = (soc_error_enum)soc_bind(pSoc->soc_id, &pSoc->addr);
		if (SOC_SUCCESS == pSoc->error)
		{
			TD_TRACE("[sock] socket udp bind successful.");
		}
		else
		{
			S8 err;
			S32 detail;
			
			soc_get_last_error(pSoc->soc_id, &err, &detail);
			TD_TRACE("[sock] socket udp bind failed, error=%d, detail=%d.", err, detail);

			return TD_ERRNO_UNKOWN_ERR;
		}
	}
	
	return TD_ERRNO_NO_ERROR;
}

S32 threedo_socket_write(THRD_SOC_MNGR *pSoc, void *buffer, unsigned int length)
{
	int wrote = 0;

	if (NULL == pSoc)
	{
		TD_TRACE("[sock] socket write, illegal parameters.");
		return TD_ERRNO_PARAM_ILLEGAL;
	}

	if (THREEDO_DNS_IGNORE != pSoc->get_ip)
	{
		TD_TRACE("[sock] socket write, split host name processing.");
		return TD_ERRNO_SOCKET_SPLITING;
	}
	
	if (SOC_SOCK_STREAM == pSoc->addr.sock_type)
	{
		wrote = soc_send(pSoc->soc_id, buffer, length, 0);
	}
	else
	{
		wrote = soc_sendto(pSoc->soc_id, buffer, length, 0, &pSoc->addr);
	}

	pSoc->error = wrote;
	TD_TRACE("[sock] socket write(%d) retern %d, error=%d.", pSoc->soc_id, wrote, pSoc->error);

	return wrote;
}

S32 threedo_socket_read(THRD_SOC_MNGR *pSoc, void *buffer, unsigned int length)
{
	int read = 0;

	if (NULL == pSoc)
	{
		TD_TRACE("[sock] socket read, illegal parameters.");
		return TD_ERRNO_PARAM_ILLEGAL;
	}

	if (THREEDO_DNS_IGNORE != pSoc->get_ip)
	{
		TD_TRACE("[sock] socket read, split host name processing.");
		return TD_ERRNO_SOCKET_SPLITING;
	}
	
	if (SOC_SOCK_STREAM == pSoc->addr.sock_type)
	{
		read = soc_recv(pSoc->soc_id, buffer, length, 0);
	}
	else
	{
		read = soc_recvfrom(pSoc->soc_id, buffer, length, 0, &pSoc->addr);
	}

	pSoc->error =  read;
	TD_TRACE("[sock] socket read(%d) return %d. error=%d.", pSoc->soc_id, read, pSoc->error);

	return read;
}

S32 threedo_socket_parse_http_head(HTTP_RSP_HEAD *out, const char *dt, U32 length)
{
	S32 temp;
	const char *pos;
	
	if (NULL == dt || NULL == out)
		return TD_ERRNO_EMPTY_PORINTER;

	if (0 == length)
		return TD_ERRNO_LENGTH_IS_ZERO;

	memset(out, 0, sizeof(HTTP_RSP_HEAD));
	//HTTP/1.1 200 OK
	if (NULL != (pos = strstr(dt, (const char*)http_version)))
	{
		pos += strlen(http_version);
		while (' ' == *pos)
			pos ++;
		
		if (1 == sscanf(pos, "%d", &temp))
		{
			out->errorCode = temp;
		}
		else
		{
			out->errorCode = 0;
		}
	}
	else
	{
		return TD_ERRNO_HTTP_ERROR_CODE;
	}

	TD_TRACE("[sock] parse http head, errorCode: %d.", out->errorCode);
	
	//HTTP头和数据之间有两个连续的回车换行符
	if (NULL != (pos = strstr((const char *)dt, (const char *)http_head_body_split)) &&
		(pos - dt) < (S32)length)
	{
		char *tagPoint;
		pos += strlen(http_head_body_split);
		out->content = (void*)pos;
		out->headLength = (U32)(pos - dt);

		//Content-Length: 1234
		out->contentLength = THREEDO_HTTP_NOT_FOUND_TAG;
		if (NULL != (pos = strstr(dt, (const char*)http_content_length)))
		{			
			pos += strlen(http_content_length) + 1;
			while (' ' == *pos)
				pos ++;

			if (1 == sscanf(pos, "%d", &temp))
			{
				out->contentLength = (U32)temp;
				TD_TRACE("[sock] parse http head, contentLength: %d.", out->contentLength);
			}
		}

		//Connection: keep-alive
		tagPoint = "Connection: keep-alive";
		out->keep_alive = THREEDO_HTTP_NOT_FOUND_TAG;
		if (NULL != (pos = strstr(dt, (const char*)tagPoint)))
		{	
			out->keep_alive = TD_TRUE;
		}
		
		//Content-Type: application/octet-stream
		tagPoint = "Content-Type: application/octet-stream";
		out->content_type = THREEDO_HTTP_NOT_FOUND_TAG;
		if (NULL != (pos = strstr(dt, (const char*)tagPoint)))
		{	
			out->content_type = THREEDO_HTTTP_CONTENT_TYPE_IS_OCTET_STREAM;
		}
		//Content-Type: application/json; charset=utf-8
		tagPoint = "Content-Type: application/json";
		if (NULL != (pos = strstr(dt, (const char*)tagPoint)))
		{	
			out->content_type = THREEDO_HTTTP_CONTENT_TYPE_IS_JSON;
		}
	}
	else
	{
		TD_TRACE("[sock] parse http head, not found split flag.");
		return TD_ERRNO_HTTP_CONTENT_POSITION;
	}

	return TD_ERRNO_NO_ERROR;
}

