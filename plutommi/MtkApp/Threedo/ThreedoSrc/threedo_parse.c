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
#include <ctype.h>
#include "string.h"
#include "stdlib.h"
#include "app_str.h"
#include "app_asyncfile.h"
#include "fs_gprot.h"


#include "threedo_errno.h"
#include "threedo_parse.h"

/*
解析配置文件 threedo.conf

文件要求:
(1)每行都要顶格写;
(2)不允许出现空格;
(3)以'#'开头的行被忽略;

文件内容如下:

#upload server configuration
uldomain=f2.yunba.io
ulport=8888
#POST /upload HTTP/1.1<CR><LF>
ulparam=/upload

#download server configuration
dldomain=f2.yunba.io
dlport=8888
#GET /files/file_name.amr HTTP/1.1<CR><LF>
dlparam=/files
dlsavefd=dlAdo

#other configuration
callto=13712345678
smsto=13812345678

#heartbeat time, unit in seconds
heartbeat=80

#APN auto adapter
apn=cmnet

*/

const char *tdConfKeyUlDomain     = "ulDomain";
const char *tdConfKeyUlParam      = "ulParam";
const char *tdConfKeyUlPort       = "ulPort";

const char *tdConfKeyDlDomain     = "dlDomain";
const char *tdConfKeyDlParam      = "dlParam";
const char *tdConfKeyDlPort       = "dlPort";
const char *tdConfKeyDlSavePath    = "dlSaveFd";

const char *tdConfKeyCallNum     = "callTo";
const char *tdConfKeySmsNum      = "smsTo";
const char *tdConfKeyHeartbeat    = "heartbeat";
const char *tdConfKeyApn         = "apn";


//http://blog.csdn.net/wesweeky/article/details/6439777
static td_parse_rslt _td_parse_line(td_kv_map *kv, char *text)
{
	S32 i;
	
	//去掉前面的空格
	while(*text && ' ' == *text)
		text++;
	
	if ('#' == *text)
		return TD_PARSED_EXPLAIN;

	memset(kv, 0, sizeof(td_kv_map));
	
	for(i=0; i<TD_KEY_STRING_LENGTH; i++)
	{
		if ('\0' == *text)
			return (i>0) ? TD_PARSED_NO_EQUAL : TD_PARSED_EMPTY;
		
		if ('=' == *text || ' ' == *text)
			break;
		
		kv->key[i] = *text++;
	}

	if (' ' == *text)
	{
		//查询是否有等号
		while(*text && ' ' == *text)
			text ++;
		
		if ('\0' == *text || '=' != *text)
			return TD_PARSED_NO_EQUAL;
	}

	text ++;//跳过'='
	
	//查询键值
	while(*text && ' ' == *text)
		text ++;
	if ('\0' == *text)
		return TD_PARSED_NO_EQUAL;

	for(i=0; i<TD_VAL_STRING_LENGTH; i++)
	{
		if ('\0' == *text)
		{
			if (i>0)
				break;
			else
				return TD_PARSED_NO_VALUE;
		}

		if (' ' == *text)
		{
			//键值里面发现空格, 是否继续
			break;
		}
		
		kv->val[i] = *text++;
	}
	
	TD_TRACE("[td parse] key=\"%s\", val=\"%s\".", kv->key, kv->val);
	return TD_PARSED_SUCCESS;
}

static void _td_fill_value(threedo_conf_info *c, td_kv_map *kv)
{
	S32 v;

	if (0 == app_stricmp(kv->key, (char*)tdConfKeyUlDomain))
	{
		strncpy(c->uldomain, kv->val, THREEDO_DOMIAN_LENGTH);
		return;
	}
	if (0 == app_stricmp(kv->key, (char*)tdConfKeyUlParam))
	{
		strncpy(c->ulrpath, kv->val, THREEDO_URL_PARAM_LENGTH);
		return;
	}
	if (0 == app_stricmp(kv->key, (char*)tdConfKeyUlPort))
	{
		if (1 == sscanf(kv->val, "%d", &v))
			c->ulport = (U16)v;
		else
			c->ulport = THREEDO_UL_PORT;
		return;
	}

	if (0 == app_stricmp(kv->key, (char*)tdConfKeyDlDomain))
	{
		strncpy(c->dldomain, kv->val, THREEDO_DOMIAN_LENGTH);
		return;
	}
	if (0 == app_stricmp(kv->key, (char*)tdConfKeyDlParam))
	{
		strncpy(c->dlrpath, kv->val, THREEDO_URL_PARAM_LENGTH);
		return;
	}
	if (0 == app_stricmp(kv->key, (char*)tdConfKeyDlPort))
	{
		if (1 == sscanf(kv->val, "%d", &v))
			c->dlport = (U16)v;
		else
			c->dlport = THREEDO_DL_PORT;
		return;
	}
	if (0 == app_stricmp(kv->key, (char*)tdConfKeyDlSavePath))
	{
		strncpy(c->dlsavefd, kv->val, THREEDO_FILE_NAME_LENGTH);
		return;
	}

	if (0 == app_stricmp(kv->key, (char*)tdConfKeyCallNum))
	{
		strncpy(c->callnum, kv->val, THREEDO_PHONE_NUM_LENGTH);
		return;
	}
	if (0 == app_stricmp(kv->key, (char*)tdConfKeySmsNum))
	{
		strncpy(c->smsnum, kv->val, THREEDO_PHONE_NUM_LENGTH);
		return;
	}
	if (0 == app_stricmp(kv->key, (char*)tdConfKeyHeartbeat))
	{
		if (1 == sscanf(kv->val, "%d", &v))
			c->heartbeat = (U16)v;
		else
			c->heartbeat = THREEDO_HEEARBEAT;
		return;
	}	
	if (0 == app_stricmp(kv->key, (char*)tdConfKeyApn))
	{
		strncpy(c->apn, kv->val, THREEDO_APN_NAME_LENGTH);
		return;
	}
}

void threedo_get_default_config_info(threedo_conf_info *out)
{
	if (NULL == out)
		return;

	memset(out, 0, sizeof(threedo_conf_info));
	
	strncpy(out->uldomain, (const char *)THREEDO_UL_DOMAIN, THREEDO_DOMIAN_LENGTH);
	strncpy(out->ulrpath, (const char *)THREEDO_UL_RPATH, THREEDO_URL_PARAM_LENGTH);
	out->ulport = THREEDO_UL_PORT;
	
	strncpy(out->dldomain, (const char *)THREEDO_DL_DOMAIN, THREEDO_DOMIAN_LENGTH);
	strncpy(out->dlrpath, (const char *)THREEDO_DL_RPATH, THREEDO_URL_PARAM_LENGTH);
	strncpy(out->dlsavefd, (const char *)THREEDO_DL_SAVE_FOLDER, THREEDO_FILE_NAME_LENGTH);
	out->dlport = THREEDO_DL_PORT;
	
	strncpy(out->apn, (const char *)THREEDO_DEFAULT_APN, THREEDO_APN_NAME_LENGTH);
	
	strncpy(out->smsnum, (const char *)THREEDO_SMS_NUMBER, THREEDO_PHONE_NUM_LENGTH);
	strncpy(out->callnum, (const char *)THREEDO_CALL_NUMBER, THREEDO_PHONE_NUM_LENGTH);
	
	out->heartbeat= THREEDO_HEEARBEAT;
}


S32 threedo_write_config_info(threedo_conf_info *info)
{
	S32 fdCard, fdPublic, drv;
	WCHAR path[32];
	char *buffer;
	U32 length;

#define PUBLIC_FS_WRITE(b, l, w)                        \
	do {                                               \
		if (fdCard >= FS_NO_ERROR)                     \
		{                                              \
			FS_Write(fdCard, (void*)(b), (l), (w));    \
		}                                              \
		if (fdPublic >= FS_NO_ERROR)                   \
		{                                              \
			FS_Write(fdPublic, (void*)(b), (l), (w));  \
		}                                              \
	} while(0)

	memset(path, 0, 32);
	drv = threedo_get_card_drive();
	kal_wsprintf((WCHAR*)path, "%c:\\%s\\%s", drv, THREEDO_FOLDER, THREEDO_CONFIG_FILE_NAME);
	fdCard = FS_Open((const WCHAR*)path, FS_READ_WRITE|FS_CREATE_ALWAYS);

	if (path[0] != (WCHAR)(drv = threedo_get_public_drive()))
	{
		path[0] = drv;
		fdPublic = FS_Open((const WCHAR*)path, FS_READ_WRITE|FS_CREATE_ALWAYS);
	}
	
	buffer = (char*)threedo_smalloc(1024);
	memset(buffer, 0, 1024);
	length = kal_snprintf((char*)buffer, 
				1023,
				"#upload\r\n"
				"%s=%s\r\n"
				"%s=%s\r\n"
				"%s=%d\r\n"
				"#download\r\n"
				"%s=%s\r\n"
				"%s=%s\r\n"
				"%s=%d\r\n"
				"%s=%s\r\n",
				tdConfKeyUlDomain, info->uldomain,
				tdConfKeyUlParam, info->ulrpath,
				tdConfKeyUlPort, info->ulport,
				tdConfKeyDlDomain, info->dldomain,
				tdConfKeyDlParam, info->dlrpath,
				tdConfKeyDlPort, info->dlport,
				tdConfKeyDlSavePath, info->dlsavefd);
	PUBLIC_FS_WRITE(buffer, length, &length);

	memset(buffer, 0, 1024);
	length = kal_snprintf((char*)buffer, 
				1023,
				"#other\r\n"
				"%s=%s\r\n"
				"%s=%s\r\n"
				"%s=%d\r\n"
				"%s=%s\r\n",
				tdConfKeyCallNum, info->callnum,
				tdConfKeySmsNum, info->smsnum,
				tdConfKeyHeartbeat, info->heartbeat,
				tdConfKeyApn, info->apn);
	PUBLIC_FS_WRITE(buffer, length, &length);
	
	if (fdCard >= FS_NO_ERROR)
	{
		FS_Commit(fdCard);
		FS_Close(fdCard);
	}

	if (fdPublic >= FS_NO_ERROR)
	{
		FS_Commit(fdPublic);
		FS_Close(fdPublic);
	}

	threedo_smfree((void *)buffer);

#undef PUBLIC_FS_WRITE

	return TD_ERRNO_NO_ERROR;
}

S32 threedo_read_config_info(threedo_conf_info *out)
{
#if 1
	strcpy(out->apn, THREEDO_DEFAULT_APN);
	strcpy(out->callnum, THREEDO_CALL_NUMBER);
	strcpy(out->dldomain, THREEDO_DL_DOMAIN);
	out->dlport = THREEDO_DL_PORT;
	strcpy(out->dlrpath, THREEDO_DL_RPATH);
	strcpy(out->dlsavefd, THREEDO_DL_SAVE_FOLDER);
	out->heartbeat = THREEDO_HEEARBEAT;
	strcpy(out->smsnum, THREEDO_SMS_NUMBER);
	strcpy(out->uldomain, THREEDO_UL_DOMAIN);
	out->ulport = THREEDO_UL_PORT;
	strcpy(out->ulrpath, THREEDO_UL_RPATH);

#else
	S32 fd;
	WCHAR buffer[THREEDO_PATH_DEEPTH+1];
	char *line;
	
	if (NULL == out)
		return TD_ERRNO_EMPTY_PORINTER;

	memset(buffer, 0, sizeof(WCHAR)*(THREEDO_PATH_DEEPTH+1));
	kal_wsprintf(buffer, "%c:\\%s\\%s", threedo_get_card_drive(), THREEDO_FOLDER, THREEDO_CONFIG_FILE_NAME);
	fd = FS_Open((const WCHAR*)buffer, FS_READ_ONLY);
	if (fd < FS_NO_ERROR)
	{
		buffer[0] = (WCHAR)threedo_get_public_drive();
		fd = FS_Open((const WCHAR*)buffer, FS_READ_ONLY);
		if (fd < FS_NO_ERROR)
		{
			//文件打不开, 就重建一个并写入一个默认值
			threedo_get_default_config_info(out);
			threedo_write_config_info(out);
			
			return TD_ERRNO_FILE_OPEN_FAILED;
		}
	}

	FS_Seek(fd, 0, FS_FILE_BEGIN);

	//开始逐行读取配置信息
	{
		U32 line_len;
		S32 ret;
		td_kv_map kv;
		
		line = (char*)buffer;
		
		do {
	        memset(line, 0, sizeof(WCHAR)*THREEDO_PATH_DEEPTH);
			line_len = 0;
	        applib_file_read_line(fd, (U8 *)line, sizeof(WCHAR)*THREEDO_PATH_DEEPTH, &line_len, &ret);

	        if(ret < FS_NO_ERROR || 0 == line_len)
	            break;
	        
			if (line_len < 3 || '#' == *line)
			{
				continue;
			}

	        //去掉尾巴上的回车换行符
	        line_len = strlen((const S8 *)line);
	        while(line_len)
	        {
	            if (line[line_len-1] == (WCHAR)'\r' || 
	                line[line_len-1] == (WCHAR)'\n' ||
	                line[line_len-1] == (WCHAR)('\r'<<8|'\n') ||
	                line[line_len-1] == (WCHAR)('\n'<<8|'\r'))
	            {
	            	line[line_len-1] = (WCHAR)'\0';
	            }
	            else
	            {
	            	break;
	            }
	            
	            line_len --;
	        }

			//解析这一行的数据信息
			if (_td_parse_line(&kv, line) >= TD_PARSED_SUCCESS)
			{
				_td_fill_value(out, &kv);
			}
	    }while(ret >= 0);
	}
	
	FS_Close(fd);
#endif

	return TD_ERRNO_NO_ERROR;
}



#if defined(__THREEDO_TEST_CODE__)
void threedo_file_parse_test(void)
{
	threedo_conf_info *info;

	info = (threedo_conf_info*)threedo_malloc(sizeof(threedo_conf_info));
	threedo_read_config_info(info);
	threedo_mfree((void *)info);
}
#endif

