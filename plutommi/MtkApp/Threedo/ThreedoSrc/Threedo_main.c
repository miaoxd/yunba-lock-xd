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

#include "SmsSrvGprot.h"
#include "BootupSrvGprot.h"
#include "SimCtrlSrvGprot.h"
#include "fs_gprot.h"
#include "NwInfoSrvGprot.h"
#include "ShutdownSrvGprot.h"
#include "DtcntSrvGprot.h"
#include "CharBatSrvGprot.h"
#include "mmi_frm_nvram_gprot.h"
#include "mmi_frm_input_gprot.h"
#include "ScrLockerGprot.h"
#include "FileMgrSrvGProt.h"
#include "SmsAppGprot.h"
#include "DtcntSrvIprot.h"
#include "DtcntSrvGprot.h"
#include "gpiosrvgprot.h"
#include "BootupSrvGprot.h"
#include "NwUsabSrvGprot.h"
#include "UcmGProt.h"
#include "UcmSrvGprot.h"
#include "DateTimeGprot.h"

#include "GlobalConstants.h"
#include "custom_data_account.h"
#include "gdi_internal.h"
#include "app_datetime.h"
#include "app_md5.h"
#include "app_base64.h"
#include "app_str.h"
#include "med_utility.h"

#include "Mmi_rp_srv_sms_def.h"
#include "mmi_rp_app_usbsrv_def.h"
#include "mmi_rp_app_scr_locker_def.h"
#include "mmi_rp_app_threedo_def.h"

#include "ThreedoGprot.h"
#include "threedo_errno.h"
#include "threedo_media.h"
#include "threedo_socket.h"
#include "threedo_parse.h"
#include "threedo_ftm.h"

char tdSmsTarget[THREEDO_PHONE_NUM_LENGTH+1];
char tdCallTarget[THREEDO_PHONE_NUM_LENGTH+1];
static WCHAR tdLogFile[32] = {0};

extern void threedo_window_initialize(void);
extern void threedo_show_version(void);
TD_BOOL threedo_runnable = TD_FALSE;

static void _td_disable_screen_lock(void)
{
	int val = 0; //referecen: MMI_SLK_SET_SELECTION_OFF
	
	//如果锁屏了，就先解锁
	if (mmi_scr_locker_is_locked())
	{
		mmi_scr_locker_close();
	}
	
	WriteValueSlim(NVRAM_SETTING_AUTOKEYPADLOCK_TIME, &val, DS_BYTE);
}

static void _td_create_home_path(void)
{
	WCHAR buffer[THREEDO_PATH_DEEPTH+1];
	S32 handle;
	S32 drive[3];
	S32 j = 0;

	drive[j++] = threedo_get_public_drive();
	drive[j++] = threedo_get_card_drive();

	for (; j>0; j--)
	{
		memset(buffer, 0, sizeof(WCHAR)*(THREEDO_PATH_DEEPTH+1));
		kal_wsprintf(buffer, "%c:\\%s", drive[j-1], THREEDO_FOLDER);
		handle = FS_Open((const WCHAR *)buffer, FS_OPEN_DIR|FS_READ_ONLY);
		if (handle >= FS_NO_ERROR)
		{
			FS_Close(handle);
		}
		else
		{
			FS_CreateDir(buffer);
		}

		{
			char *items[10];
			int i=0;

			items[i++] = THREEDO_DOWNLOAD_FOLER;
			items[i++] = THREEDO_AUDIO_FOLER;
			
			//items[i++] = THREEDO_IMAGE_FOLER;
			//items[i++] = THREEDO_VIDEO_FOLER;
		
			for (; i>0; i--)
			{
				memset(buffer, 0, sizeof(WCHAR)*(THREEDO_PATH_DEEPTH+1));
				kal_wsprintf(buffer, "%c:\\%s\\%s", drive[j-1], THREEDO_FOLDER, items[i-1]);
				handle = FS_Open((const WCHAR *)buffer, FS_OPEN_DIR|FS_READ_ONLY);
				if (handle >= FS_NO_ERROR)
				{
					FS_Close(handle);
				}
				else
				{
					FS_CreateDir(buffer);
				}
			}
		}
	}
}

/*******************************************************************************
** 函数: threedo_create_path
** 功能: 创建一级深度的路径
** 参数: drive   -- 盘符
**       folder  -- 文件夹名字
** 返回: 是否创建成功
** 作者: threedo
*******/
S32 threedo_create_path(S32 drive, const WCHAR *folder)
{
    S32 h;
    WCHAR buffer[THREEDO_PATH_DEEPTH+1] = {0};

    if (/*!isalpha(drive) ||*/ !srv_fmgr_drv_is_accessible(drive) || NULL == folder)
        return TD_FALSE;
    
    kal_wsprintf(buffer, "%c:\\%w\\", drive, folder);
	if ((h = FS_Open((const WCHAR*)buffer, FS_OPEN_DIR|FS_READ_ONLY)) >= FS_NO_ERROR)
    {
        FS_Close(h);
        return TD_TRUE;
    }
	else
    {
        return (S32)(FS_CreateDir((const WCHAR*)buffer) >= FS_NO_ERROR);
    }
}

/*******************************************************************************
** 函数: threedo_create_path_multi
** 功能: 创建路径, 可以是多重路径
** 参数: dirve     -- 盘符
**       UcsFolder -- 文件夹
** 返回: 无
** 作者: threedo
*******/
S32 threedo_create_path_multi(S32 drive, const WCHAR *UcsFolder)
{
    U32 i, slash;
    WCHAR *path, ch;
    S32 fd;
 
    
    if (/*!isalpha(drive) || */
        !srv_fmgr_drv_is_accessible(drive) ||
        NULL == UcsFolder)
        return TD_FALSE;

    path    = (WCHAR*)threedo_smalloc((THREEDO_PATH_DEEPTH+1)*sizeof(WCHAR));
	memset(path, 0, (THREEDO_PATH_DEEPTH+1)*sizeof(WCHAR));
	
    path[0] = (WCHAR)drive;
    path[1] = ':';
    path[2] = '\\';

    i       = 0;
    fd      = FS_NO_ERROR;
    
    do {
        //  d:\a\b\c\d\e
        //  d:\\a\b\c\\

        slash   = 1;
        
        while((ch = *UcsFolder++) != (WCHAR)'\0')
        {
            if (ch == '\\' || ch == '/')
            {
                if (0 == slash)
                {
                    path[3+i] = ch;
                    slash = 2;
                    i ++;
                    break;
                }
                continue;
            }
            else
            {
                if (2 == slash)
                {
                    i ++;
                    break;
                }
                
                path[3+i] = ch;
                slash = 0;
            }

            i ++;
            
            if (3+i >= (THREEDO_PATH_DEEPTH-2))
                break;
        }

        if (ch != '\\' && ch != '/' && path[3+i-1] != '\\' && path[3+i-1] != '/')
        {
            path[3+i] = '/';
            i ++;
        }
        
        //create
        fd = FS_Open((const WCHAR*)path, FS_OPEN_DIR|FS_READ_ONLY);
        if (fd >= FS_NO_ERROR)
            FS_Close(fd);
    	else if ((fd = FS_CreateDir((const WCHAR*)path)) < FS_NO_ERROR)
            break;
        
        if (ch == (WCHAR)'\0')
            break;
            
    }while(3+i < (THREEDO_PATH_DEEPTH-2));

    threedo_smfree((void*)path);
    
    return (S32)(fd >= FS_NO_ERROR);
}

/*******************************************************************************
** 函数: threedo_create_path_multi_ex
** 功能: 创建路径, 可以是多重路径
** 参数: path 路径
** 返回: 无
** 作者: threedo
*******/
S32 threedo_create_path_multi_ex(const WCHAR *path)
{
	S32 fd;
	if (FS_NO_ERROR > (fd = FS_Open(path, FS_OPEN_DIR|FS_READ_ONLY)))
	{
		return threedo_create_path_multi((S32)path[0], (const WCHAR*)(path+3));
	}
	
	FS_Close(fd);
	
	return TD_TRUE;
}

/*******************************************************************************
** 函数: threedo_log
** 功能: 打印日志
** 参数: fmt
** 返回: 无
** 作者: threedo
*******/
S32 threedo_log(const char *fmt, ...)
{
#if defined(WIN32)
#define LOG_FILE_LIMITED     (1024*8)  //KB
#else
#define LOG_FILE_LIMITED     (1024*4)  //KB
#endif

	static S32 logSizeCount = -1;
	static S32 logFH = 0;
	static char buffer[256];
	
	S32 length;
	U32 written = 0;
	va_list list;

#if defined(__MMI_USB_SUPPORT__) && defined(__USB_IN_NORMAL_MODE__)
	if (srv_usb_is_in_mass_storage_mode())
		return 0;
#endif

	if (-1 == logSizeCount)
	{
		memset(tdLogFile, 0, sizeof(tdLogFile));
		kal_wsprintf(tdLogFile, "%c:\\%s\\tdlog.txt", threedo_get_usable_drive(), THREEDO_FOLDER);
		FS_Delete(tdLogFile);
	}
	else if (logSizeCount >= LOG_FILE_LIMITED*1024)
	{		
		if (logFH > FS_NO_ERROR)
			FS_Close(logFH);		
		
		FS_Delete(tdLogFile);			
		logSizeCount = 0;
	}
	
	if (logFH <= FS_NO_ERROR)
	{
		const char *logHead = "\n==>\n";
				
		logFH = FS_Open((const WCHAR*)tdLogFile, FS_READ_WRITE|FS_CREATE);
		if (logFH <= FS_NO_ERROR)
			return 0;

		FS_Seek(logFH, 0, FS_FILE_END);
		FS_Write(logFH, (void*)logHead, strlen(logHead), &written);
		logSizeCount += (S32)written;
	}
	
	va_start(list, fmt);
#if defined(WIN32)
	length = _vsnprintf(buffer, sizeof(buffer), fmt, list);
#else
	length = vsnprintf(buffer, sizeof(buffer), fmt, list);
#endif
	va_end(list);

	FS_Write(logFH, buffer, length, &written);
	logSizeCount += (S32)written;

	{
		applib_time_struct t;
		
		applib_dt_get_rtc_time(&t);
		length = sprintf(buffer, "[%02d:%02d:%02d]\n", t.nHour, t.nMin, t.nSec);
		FS_Write(logFH, buffer, length, &written);
		logSizeCount += (S32)written;
	}
	
	FS_Commit(logFH);

	return logSizeCount;
}


void *threedo_malloc(U32 size)
{
#if 1
	return med_alloc_ext_mem(size);
#else
	return gdi_alloc_ext_mem(size);
#endif
}

void threedo_mfree(void *ptr)
{
#if 1
	med_free_ext_mem((void**)&ptr);
#else
	gdi_free_ext_mem((void**)&ptr);
#endif
}

void *threedo_smalloc(U16 size)
{
	return OslMalloc(size);
}

void threedo_smfree(void *ptr)
{
	OslMfree(ptr);
}

void threedo_send_message(U16 msg_id, void *req, int mod_src, int mod_dst, int sap)
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

/*******************************************************************************
** 函数: threedo_send_message_to_mmi
** 功能: 发送消息到MMI模块
** 参数: msgid  -- 消息ID
**       msg    -- 消息
** 返回: 无
** 作者: threedo
*******/
void threedo_send_message_to_mmi(msg_type msgid, void *msg)
{
    MYQUEUE message;
    
    message.oslMsgId = msgid;
    message.oslDataPtr = (local_para_struct *)msg;
    message.oslPeerBuffPtr = NULL;
    message.oslSrcId = MOD_MMI;
    message.oslDestId = MOD_MMI;
    mmi_msg_send_ext_queue(&message);  
}

/*******************************************************************************
** 函数: threedo_construct_para
** 功能: 参数构造buffer
** 参数: sz   表示需要使用的buffer大小.
** 返回: 无
** 作者: threedo
*******/
void *threedo_construct_para(U32 sz)
{
    return construct_local_para(sz, TD_CTRL | TD_RESET);
}

/*******************************************************************************
** 函数: threedo_destruct_para
** 功能: 参数buffer释放
** 参数: ptr   表示需要释放的参数buffer地址.
** 返回: 无
** 作者: threedo
*******/
void threedo_destruct_para(void *ptr)
{
    free_local_para((local_para_struct*)ptr);
}

void threedo_md5(char *out, U8 *data, U32 length)
{
    applib_md5_ctx md5_ctx;

    applib_md5_init(&md5_ctx);
    applib_md5_update(&md5_ctx, data, length);
    applib_md5_final(out, &md5_ctx);
}

void threedo_md5_file(char *out, const WCHAR *file)
{
#if 1
	strcpy(out, "hereiso_md5_file....");
#else
	S32 fd;
	
	fd = FS_Open(file, FS_READ_ONLY);
	if (fd > FS_NO_ERROR)
	{
		U8 *buffer;
		U32 read;
		U32 size = 4096;
		applib_md5_ctx md5_ctx;

		buffer = (U8*)threedo_malloc(size);
		do {
			FS_Read(fd, (void*)buffer, size, &read);
			if (read)
			{
				applib_md5_init(&md5_ctx);
				applib_md5_update(&md5_ctx, buffer, read);
    			applib_md5_final(out, &md5_ctx);
				break;
			}
		}while(read);	

		FS_Close(fd);
		threedo_mfree(buffer);
	}
#endif
}

void threedo_base64(char *out, U32 out_length, U8 *src, U32 src_len)
{	
   applib_base64_encode((S8*)src, src_len, (S8*)out, out_length);
}

void threedo_base64_file(char *out, U32 out_length, const WCHAR *file)
{

}

U32 threedo_crc32(U8 *data, U32 length)
{
    const unsigned int crc32_table[]={ /* CRC polynomial 0xedb88320 */
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
		0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 
		0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
        0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
        0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
        0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
        0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
        0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
        0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
        0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
        0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
        0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
        0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
        0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
        0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
        0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
        0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
        0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };
    register unsigned int oldcrc32, idx;
		
    for (idx=oldcrc32=0; idx<length; idx++)
    {
        oldcrc32 = ((oldcrc32 << 8) | data[idx]) ^ (crc32_table[(oldcrc32 >> 24) & 0xFF]);
    }

	//trace_v("[CRC32] return 0x%x. data_length=%d.", oldcrc32, len);
    return oldcrc32;
}

U32 threedo_crc32_file(const WCHAR *file)
{
	S32 fd;
	U32 crc = 0;
	return 0;
	fd = FS_Open(file, FS_READ_ONLY);
	if (fd > FS_NO_ERROR)
	{
		U8 *buffer;
		U32 read;
		
		buffer = (U8*)threedo_malloc(4096);
		do {
			memcpy(buffer, &crc, 4);
			FS_Read(fd, (void*)(buffer+4), 4096-4, &read);
			if (read)
			{
				crc = threedo_crc32(buffer, read+4);
			}
		}while(read > 0);

		FS_Close(fd);
		threedo_mfree(buffer);
	}

	return crc;
}

S32 threedo_get_usable_sim_index(void)
{
	if (threedo_is_sim_usable(MMI_SIM1))
		return 0;

	if (threedo_is_sim_usable(MMI_SIM2))
		return 1;

	TD_TRACE("[threedo] [WARN] no SIM card can be use now.");
	return -1;
}

/*******************************************************************************
** 函数: threedo_get_system_tick
** 功能: 获取系统TICK
** 参数: 无
** 返回: 无
** 作者: threedo
*******/
U32 threedo_get_system_tick(void)
{
#if defined(__MTK_TARGET__)
	return kal_get_systicks();
#else
	return app_getcurrtime(); 
#endif
}

/*******************************************************************************
** 函数: threedo_get_current_time
** 功能: 获取系统当前的运行时间
** 参数: 无
** 返回: 运行时间
** 作者: threedo
*******/
U32 threedo_get_current_time(void)
{
    return app_getcurrtime();
}

/*******************************************************************************
** 函数: threedo_get_date_time
** 功能: 获取系统当前的时间
** 参数: 时间结构体
** 返回: 无
** 作者: threedo
*******/
void threedo_get_date_time(applib_time_struct *t)
{    
    applib_dt_get_date_time(t);
}

/*******************************************************************************
** 函数: threedo_set_date_time
** 功能: 设置系统当前的时间
** 参数: 时间结构体
** 返回: 无
** 作者: threedo
*******/
void threedo_set_date_time(applib_time_struct *t)
{
    mmi_dt_set_rtc_dt((MYTIME *)t);
}

/*******************************************************************************
** 函数: threedo_get_disk_free_size
** 功能: 获取磁盘剩余空间
** 参数: drive, 磁盘盘符, 0x43~0x47
** 返回: 剩余空间的大小, byte为单位
** 作者: threedo
*******/
U32 threedo_get_disk_free_size(S32 drive)
{
    FS_DiskInfo disk_info;
    WCHAR path[8] = {0};
    U32 size;

    kal_wsprintf(path, "%c:\\", drive);
    
    if (FS_GetDiskInfo(
            path,
            &disk_info,
            FS_DI_BASIC_INFO | FS_DI_FREE_SPACE) >= FS_NO_ERROR)
    {
        size = disk_info.FreeClusters * disk_info.SectorsPerCluster * disk_info.BytesPerSector;
        return size;
    }

    return 0;
}

/*******************************************************************************
** 函数: threedo_is_sim_usable
** 功能: 判断SIM卡是否可以使用, 仅当SIM卡读取正常且校验都通过了才算可以使用.
** 参数: sim  -- SIM卡索引
** 返回: 是否可以使用
** 作者: threedo
*******/
S32 threedo_is_sim_usable(mmi_sim_enum sim)
{
	S32 exist;
	
	if (TD_FALSE == (exist = srv_sim_ctrl_is_available(sim)))
    {   
	    TD_TRACE("[threedo] sim=0x%x. not available.", sim);
    }
    
	return exist;
}

/*******************************************************************************
** 函数: threedo_get_usable_drive
** 功能: 获取系统可用的存储设备盘符
** 参数: 无
** 返回: 返回盘符 0x43~0x47
** 作者: threedo
*******/
S32 threedo_get_usable_drive(void)
{
    S32 drive = SRV_FMGR_CARD_DRV;

    if(!srv_fmgr_drv_is_accessible(drive))
    {
        drive = SRV_FMGR_PUBLIC_DRV;
    }

    return drive;
}


S32 threedo_is_tcard_exist(void)
{
	return (S32)srv_fmgr_drv_is_accessible(SRV_FMGR_CARD_DRV);
}

S32 threedo_is_drive_usable(S32 drive)
{
	return (S32)srv_fmgr_drv_is_accessible(drive);
}

S32 threedo_get_card_drive(void)
{
    return FS_GetDrive(FS_DRIVE_V_REMOVABLE, 1, FS_NO_ALT_DRIVE);
}

S32 threedo_get_public_drive(void)
{
    return FS_GetDrive(FS_DRIVE_V_NORMAL, 2, FS_DRIVE_V_NORMAL | FS_DRIVE_V_REMOVABLE);
}

S32 threedo_get_private_drive(void)
{
    return FS_GetDrive(FS_DRIVE_I_SYSTEM, 2, FS_ONLY_ALT_SERIAL );
}

void threedo_clear_message_protocol(U16 msgid, THREEDO_MSG_CBF funcPtr)
{
	mmi_frm_clear_protocol_event_handler(msgid, (PsIntFuncPtr)funcPtr);
}

void threedo_set_message_protocol(U16 msgid, THREEDO_MSG_CBF funcPtr)
{
	mmi_frm_set_protocol_event_handler(msgid, (PsIntFuncPtr)funcPtr, TD_FALSE);
}

void threedo_set_message_protocol_multi(U16 msgid, THREEDO_MSG_CBF funcPtr)
{
	mmi_frm_set_protocol_event_handler(msgid, (PsIntFuncPtr)funcPtr, TD_TRUE);
}

WCHAR *threedo_find_file_name_position(WCHAR *path)
{
	return threedo_get_file_name_from_path(path);
}

WCHAR *threedo_find_file_suffix_name_position(WCHAR *path)
{
	S32 temp;
	
	if (NULL == path)
		return NULL;

	temp = app_ucs2_strlen((const S8*)path);
	while ('.' != *(path+temp))
		temp --;

	return (path+temp);
}

WCHAR *threedo_get_file_name_from_path(WCHAR *path)
{
	WCHAR *pos;
		
	pos = path + app_ucs2_strlen((const S8 *)path) - 1;
	while (pos > path && '\\' != *pos)
		pos --;

	return pos+1;
}

//默认的数据账户APP类型集, 应该允许设置成默认, 允许MMS, 允许WAP/HTTP浏览器.
#define DEFAULT_DTCNT_APPTYPE_SET (DTCNT_APPTYPE_DEF|DTCNT_APPTYPE_MMS|DTCNT_APPTYPE_BRW_WAP|DTCNT_APPTYPE_BRW_HTTP)
//默认的数据账户索引ID
#define DEFAULT_DTCNT_PROFILE_ID  0

#if 0
/*******************************************************************************
** 函数: msgcmd_SetWAPActivedAccountCb
** 功能: 设置WAP的数据账户信息的回调函数
** 参数: p -- srv_dtcnt_prof_common_header_struct
** 返回: 是否设置成功.
** 说明: prof->prof_fields 参见 dtcntsrviprot.h 中的 SRV_DTCNT_PROF_FIELD_xxx.
**       acc_info->prof_common_header.user_data填PLMN, 如"46000, 46002, 46007".
**       但是由于这个user_data指向一个const数据, 所以，我们无法填写.
** 作者: wasfayu
*******/
void msgcmd_SetWAPActivedAccountCb(S32 user_data, U32 result, void *obj)
{	
	threedo_mfree((void*)user_data);
}


/*******************************************************************************
** 函数: msgcmd_SetWAPActivedAccount
** 功能: 设置WAP的数据账户信息
** 参数: p -- srv_dtcnt_prof_common_header_struct
** 返回: 是否设置成功.
** 说明: 参考 srv_brw_set_profile_req.
** 作者: wasfayu
*******/
static void msgcmd_SetWAPActivedAccount(srv_dtcnt_prof_common_header_struct *p)
{
	srv_brw_set_profile_req_struct *set_profile;
	srv_brw_act_req_struct obj = {0};
	cbm_sim_id_enum cbm_sim_id = CBM_SIM_ID_SIM1;
	mmi_wap_prof_sim_id_enum wap_sim_id = MMI_WAP_PROF_SIMID_SIM1;
	U8 app_id = 0;
	U32 encode_acc_id = 0;

	if (0 == threedo_get_usable_sim_index())
	{
		 cbm_sim_id = CBM_SIM_ID_SIM1;
		 wap_sim_id = MMI_WAP_PROF_SIMID_SIM1;
		 app_id = g_mmi_wap_prof_cntx.cbm_brw_app_id_1;
	}
	else if (1 == threedo_get_usable_sim_index())
	{
		cbm_sim_id = CBM_SIM_ID_SIM2;
		wap_sim_id = MMI_WAP_PROF_SIMID_SIM2;
		app_id = g_mmi_wap_prof_cntx.cbm_brw_app_id_2;
	}
	else
	{
		ASSERT(0);
	}

	encode_acc_id = cbm_encode_data_account_id(p->acc_id, cbm_sim_id, app_id, FALSE);
	
	set_profile = (srv_brw_set_profile_req_struct *)threedo_malloc(sizeof(*set_profile));
    set_profile->setting_id = wap_bam_setting_type_both;
	set_profile->currprof = mmi_wap_prof_get_profile_content(
                                    wap_sim_id,
                                    MMI_WAP_PROF_APPID_BRW,
                                    (U8) - 1,
                                    p->acc_id,
                                    NULL);
	set_profile->currprof->conn_type = MMA_CONN_TYPE_HTTP_PROXY;
	set_profile->currprof->data_account_primary_id = encode_acc_id;
	set_profile->currprof->proxy_port = p->px_port;
	set_profile->currprof->proxy_ip[0] = p->px_addr[0];
    set_profile->currprof->proxy_ip[1] = p->px_addr[1];
    set_profile->currprof->proxy_ip[2] = p->px_addr[2];
    set_profile->currprof->proxy_ip[3] = p->px_addr[3];
    strcpy((S8 *) set_profile->currprof->url, (S8 *) p->HomePage);
    strcpy((S8 *) set_profile->currprof->username, (S8 *) p->Auth_info.UserName);
    strcpy((S8 *) set_profile->currprof->password, (S8 *) p->Auth_info.Passwd);

	
	g_cui_dtcnt_sel_index = 0;
	g_cui_dtcnt_sel_acct[g_cui_dtcnt_sel_index].account_id = encode_acc_id;
	g_mmi_wap_prof_cntx.active_dtcnt_index[wap_sim_id][MMI_WAP_PROF_APPID_BRW] = encode_acc_id;
	mmi_wap_prof_nvram_write_active_profile_index(wap_sim_id, MMI_WAP_PROF_APPID_BRW);

	/* 参考: 
	   (1) MMSBGSRMsg.c文件中的mmi_mms_bgsr_set_profile_req函数;
	   (2) WAPprofilemain.c文件中的mmi_wap_prof_activate_profile函数. */
	obj.rsp_callback = msgcmd_SetWAPActivedAccountCb;
	obj.req_data = (void*)set_profile;
	obj.user_data= (U32)set_profile;
    srv_brw_set_profile_req_by_data(&obj);
}


/*******************************************************************************
** 函数: threedo_set_data_account
** 功能: 设置GPRS数据账户信息请求
** 参数: acc_info -- 数据账户的数据结构 srv_dtcnt_prof_gprs_struct.
** 返回: 无.
** 说明: prof->prof_fields 参见 dtcntsrviprot.h 中的 SRV_DTCNT_PROF_FIELD_xxx.
** 作者: threedo
*******/
S32 threedo_set_data_account(srv_dtcnt_prof_gprs_struct *acc_info)
{
	srv_dtcnt_store_prof_data_struct prof;
	srv_dtcnt_result_enum ret = SRV_DTCNT_RESULT_FAILED;
	U32 actual_id = DEFAULT_DTCNT_PROFILE_ID;
	
	if (NULL == acc_info)
		return MMI_FALSE;

	memset(&prof, 0, sizeof(srv_dtcnt_store_prof_data_struct));
	prof.prof_type = SRV_DTCNT_BEARER_GPRS;
	prof.prof_fields = SRV_DTCNT_PROF_FIELD_ALL;
	prof.prof_data = (void*)acc_info;

	if (0 != strlen(acc_info->APN))
	{
		srv_dtcnt_account_info_st *query_ret;
		
		// 先查询下APN是否存在, 如果存在则修改这个APN对应的ID, 否则就新增加一个.
		query_ret= srv_dtcnt_db_store_get_acc_info_by_apn((S8*)acc_info->APN);	
		if (NULL != query_ret)
		{
			actual_id = query_ret->acc_id;
			TD_TRACE("[threedo] account:\"%s\" is exist, exsit_id=%d.", acc_info->APN, actual_id);
			
			acc_info->prof_common_header.acc_id = actual_id;
			acc_info->prof_common_header.AppType |= DEFAULT_DTCNT_APPTYPE_SET;
			
			ret = srv_dtcnt_store_update_prof(actual_id, &prof);
		}
		else
		{
			//增加			
			acc_info->prof_common_header.acc_id = DEFAULT_DTCNT_PROFILE_ID;
			acc_info->prof_common_header.AppType |= DEFAULT_DTCNT_APPTYPE_SET;

			ret = srv_dtcnt_store_add_prof(&prof, &actual_id);
			if (SRV_DTCNT_RESULT_SUCCESS != ret)
			{
				//如果直接插入失败了, 那就再执行一次更新得了.
				TD_TRACE("[threedo] account:\"%s\" is not exist, add failed, update account %d.", acc_info->APN, actual_id);
				ret = srv_dtcnt_store_update_prof(actual_id, &prof);
			}
			else
			{
				TD_TRACE("[threedo] account:\"%s\" is not exist, add one item.", acc_info->APN);
				mmi_dtcnt_add_disp_list(
					actual_id, 
					DATA_ACCOUNT_BEARER_GPRS, 
					SRV_DTCNT_PROF_TYPE_USER_CONF,
					1,
					acc_info->prof_common_header.sim_info,
					acc_info->prof_common_header.read_only);
			}
		}
		
		// 先调用SRV函数将数据存储到文件(NVRAM)中, 再调用MMI的函数更新
		if (SRV_DTCNT_RESULT_SUCCESS == ret)
		{
			srv_dtcnt_prof_common_header_struct *prof_head;
			srv_dtcnt_set_default_acc(actual_id);
			mmi_dtcnt_set_account_default_app_type(actual_id, MMI_FALSE);
			mmi_dtcnt_update_disp_list(actual_id, 
				DATA_ACCOUNT_BEARER_GPRS, 
				SRV_DTCNT_PROF_TYPE_USER_CONF, 
				0, 
				acc_info->prof_common_header.sim_info,
				acc_info->prof_common_header.read_only);
			msgcmd_SetWAPActivedAccount(&acc_info->prof_common_header);
		}
		
		TD_TRACE("[threedo] account:\"%s\",%d.%d.%d.%d:%d,\"%s\",\"%s\",0x%x,\"%s\".",
			acc_info->APN,
			acc_info->prof_common_header.px_addr[0],
			acc_info->prof_common_header.px_addr[1],
			acc_info->prof_common_header.px_addr[2],
			acc_info->prof_common_header.px_addr[3],
			acc_info->prof_common_header.px_port,
			acc_info->prof_common_header.Auth_info.UserName,
			acc_info->prof_common_header.Auth_info.Passwd,
			acc_info->prof_common_header.acc_id,
			acc_info->prof_common_header.HomePage);
	}


	TD_TRACE("[threedo] account set, id=%d, apn=\"%s\", err=%d.", actual_id, acc_info->APN, ret);
	
	return (S32)(SRV_DTCNT_RESULT_SUCCESS == ret);
}
#endif

S32 threedo_make_call(const char *pnumber, S32 simid)
{
#ifdef __MMI_DUAL_SIM_MASTER__
	mmi_ucm_make_call_para_struct param;
#endif
	WCHAR Hotline_number[(20+1)*2] = {
				0x31,0x00, // 1
				0x31,0x00, // 1
				0x32,0x00, // 2
				0x00,0x00
				};

    if (NULL == pnumber)
        return TD_ERRNO_PARAM_ILLEGAL;
    
	
	if (mdi_audio_is_recording() || mdi_audio_is_playing(MDI_AUDIO_PLAY_ANY))
		return TD_ERRNO_MODULE_BUSING;

    app_asc_str_to_ucs2_wcs(Hotline_number, (S8*)pnumber);
	
#ifndef __MMI_DUAL_SIM_MASTER__ 
	// MakeCall(Hotline_number);
#else
    if (!srv_nw_usab_is_any_network_available())
    {
        TD_TRACE("[threedo] make call(%s) by sim(%d), network not available.", pnumber, simid);
        return TD_ERRNO_NW_NOT_USABLE;
    }

	if (mdi_audio_is_recording() || mdi_audio_is_playing(MDI_AUDIO_PLAY_ANY))
		return TD_ERRNO_MODULE_BUSING;

    TD_TRACE("[threedo] make call(%s) by sim(%d).", pnumber, simid);
	mmi_ucm_init_call_para_for_sendkey(&param); 
	
	param.dial_type = (1==simid) ? SRV_UCM_SIM2_CALL_TYPE_ALL : SRV_UCM_SIM1_CALL_TYPE_ALL;
	
	param.ucs2_num_uri = (U16 *)Hotline_number;
    mmi_ucm_call_launch(0, &param);
#endif

	return TD_ERRNO_NO_ERROR;
}

S32 threedo_send_sms(const WCHAR *ucs_text, const char *number, S32 simid)
{
    SMS_HANDLE sms_handle;
    S8 ucs2_number[(SRV_SMS_MAX_ADDR_LEN + 1)*ENCODING_LENGTH] = {0};
    
    
	if (!mmi_sms_is_sms_ready() ||
        !srv_mode_switch_is_network_service_available() ||
        NULL == number ||
        NULL == ucs_text)
	{
		TD_TRACE("[threedo] send sms to(%s) by sim(%d). network not available", number, simid);
	    return TD_ERRNO_NW_NOT_USABLE;
    }
    TD_TRACE("[threedo] send sms to(%s) by sim(%d).", number, simid);

	app_asc_str_to_ucs2_str((char*)ucs2_number, (char*)number);
    
    sms_handle = srv_sms_get_send_handle();
    //srv_sms_set_not_allow_empty_sc(sms_handle);
	srv_sms_set_address(sms_handle, (char*)ucs2_number);
    srv_sms_set_content_dcs(sms_handle, SRV_SMS_DCS_UCS2);
    srv_sms_set_sim_id(sms_handle, (1 == simid) ? SRV_SMS_SIM_2 : SRV_SMS_SIM_1);
    srv_sms_set_send_type(sms_handle, SRV_SMS_BG_SEND);
    //srv_sms_set_counter_without_change(sms_handle);
    srv_sms_set_no_sending_icon(sms_handle);
    srv_sms_set_content(sms_handle, (char*)ucs_text, app_ucs2_strlen((const S8*)ucs_text)*sizeof(WCHAR));
    
    srv_sms_send_msg(sms_handle, NULL, NULL);

    return TD_ERRNO_NO_ERROR;
}

S32 threedo_play_audio(const WCHAR *ado_path)
{
	return threedo_audio_play(ado_path, NULL);
}

S32 threedo_process_key(S16 key_code, S16 key_type)
{
	S32 proced = TD_FALSE;

	if (!srv_bootup_is_completed())
		return TD_FALSE;

	//TD_TRACE("[threedo] key process, code=%d, type=%d", key_code, key_type);	
	switch (key_code)
	{
	#if defined(ENABLE_SYSTEM_KEY_FUNCTION)
	case KEY_END:
		if (KEY_EVENT_UP == key_type)
		{
			srv_backlight_is_on(SRV_BACKLIGHT_TYPE_MAINLCD) ? 
				srv_backlight_close() : srv_backlight_turn_on(SRV_BACKLIGHT_SHORT_TIME);
		}
		proced = TD_TRUE;
		break;

	case KEY_1:
		if (KEY_LONG_PRESS == key_type)
		{
			//make a call
			S32 simindex = threedo_get_usable_sim_index();
			if (simindex >= 0)
			{
				threedo_make_call(tdCallTarget, simindex);
			}
			proced = TD_TRUE;
		}
		break;

	case KEY_2:
		if (KEY_LONG_PRESS == key_type)
		{
			WCHAR buffer[32];
			applib_time_struct dt;
			S32 simindex = threedo_get_usable_sim_index();
			
			applib_dt_get_date_time(&dt);
			memset(buffer, 0, sizeof(buffer));
			kal_wsprintf(buffer, "Time: %04d-%02d-%02d,%02d:%02d:%02d",
				dt.nYear, dt.nMonth, dt.nDay, dt.nHour, dt.nMin, dt.nSec);

			if (simindex >= 0)
			{
				threedo_send_sms((const WCHAR *)buffer, (const char *)tdSmsTarget, simindex);
			}
			proced = TD_TRUE;
		}
		break;
	#endif
	
	case KEY_CSK:
		if (threedo_get_usable_sim_index() >= 0 &&
			threedo_runnable == TD_TRUE)
		{	
			if (KEY_EVENT_DOWN == key_type)
			{
				//start
				threedo_custom_audio_record_start();
			}
			else if (KEY_EVENT_UP == key_type)
			{
				//stop
				threedo_custom_audio_record_stop();
			}
			proced = TD_TRUE;
	
			break;
		}

	default:
		proced = TD_FALSE;
		break;
	}

	return proced;//return TD_FALSE; //如果不想让后面处理则返回0
}

S32 threedo_process_secure_code(WCHAR *dial_string)
{
	if (0 == mmi_ucs2cmp((CHAR*)dial_string, (CHAR*)L"*83*99#"))
	{
		threedo_show_version();
		return TD_TRUE;
	}
	
	if (0 == mmi_ucs2cmp((CHAR*)dial_string, (CHAR*)L"*83*00#") &&
		threedo_runnable == TD_TRUE)
	{
		//这个用来停止任务
		threedo_send_message(
			MSG_ID_THREEDO_TASK_STOP_REQ,
			NULL,
			MOD_MMI,
			MOD_THREEDO,
			0);
		threedo_runnable = TD_FALSE;
		return TD_TRUE;
	}

	if (0 == mmi_ucs2cmp((CHAR*)dial_string, (CHAR*)L"*83*01#") &&
		threedo_runnable == TD_FALSE)
	{
		threedo_runnable = TD_TRUE;

		kal_prompt_trace(MOD_MQTT, "======1\n");
		threedo_send_message(MSG_ID_MQTT_START, NULL, 
				MOD_MMI, MOD_MQTT, 0);
		
		//这个用来启动任务, 暂时不做处理
		threedo_send_message(
			MSG_ID_THREEDO_TASK_START_REQ,
			NULL,
			MOD_MMI,
			MOD_THREEDO,
			0);

		return TD_TRUE;
	}

	return TD_FALSE;
}

mmi_ret threedo_process_system_event(mmi_event_struct *evt)
{
	MMI_RET ret = MMI_RET_OK;
	
	switch(evt->evt_id)
	{
	case EVT_ID_SRV_BOOTUP_EARLY_INIT:
		TD_TRACE("========Threedo studio=======");
		memset(tdSmsTarget, 0, THREEDO_PHONE_NUM_LENGTH+1);
		memset(tdCallTarget, 0, THREEDO_PHONE_NUM_LENGTH+1);
		break;
		
	case EVT_ID_SRV_BOOTUP_COMPLETED:
		{
			threedo_conf_info out;
			threedo_read_config_info(&out);
			strncpy(tdSmsTarget, (const char *)out.smsnum, THREEDO_PHONE_NUM_LENGTH);
			strncpy(tdCallTarget, (const char *)out.callnum, THREEDO_PHONE_NUM_LENGTH);
			
			_td_disable_screen_lock();
			threedo_media_initialize();
			threedo_file_trans_init();
			threedo_window_initialize();
		}
		break;
		
	case EVT_ID_SRV_BOOTUP_BEFORE_IDLE:
		break;
		
	case EVT_ID_SRV_BOOTUP_NORMAL_INIT:
		_td_create_home_path();		
		break;
		
	case EVT_ID_SRV_SMS_READY:
		threedo_send_message(MSG_ID_THREEDO_TASK_START_REQ, NULL, MOD_MMI, MOD_THREEDO, 0);
		break;

	case EVT_ID_SCR_LOCKER_LOCKED: //macro: __MMI_AUTO_KEYPAD_LOCK__
		_td_disable_screen_lock();
		break;
		
#ifdef __MMI_USB_SUPPORT__
    case EVT_ID_USB_ENTER_MS_MODE:
		break;
	case EVT_ID_USB_EXIT_MS_MODE:
		break;
#endif

	/* network infomation */
	case EVT_ID_SRV_NW_INFO_STATUS_CHANGED:
		break;
	case EVT_ID_SRV_NW_INFO_SERVICE_AVAILABILITY_CHANGED:
		break;
	case EVT_ID_SRV_NW_INFO_SIGNAL_STRENGTH_CHANGED:
		break;
	case EVT_ID_SRV_NW_INFO_LOCATION_CHANGED:
		//srv_nw_info_location_changed_evt_struct, 获取基站信息
		//或者响应 MSG_ID_MMI_NW_ATTACH_IND 这个消息
		break;
	case EVT_ID_SRV_NW_INFO_ROAMING_STATUS_CHANGED:
		break;
	case EVT_ID_SRV_NW_INFO_PROTOCOL_CAPABILITY_CHANGED:
		break;
	case EVT_ID_SRV_NW_INFO_SIM_DN_STATUS_CHANGED:
		break;
	case EVT_ID_SRV_NW_INFO_REGISTER_FAILED:
		break;
	
	/* SIM contral */
	case EVT_ID_SRV_SIM_CTRL_AVAILABILITY_CHANGED:
		{
			srv_nw_info_service_availability_changed_evt_struct *mEvt = 
				(srv_nw_info_service_availability_changed_evt_struct *)evt;

			if (SRV_NW_INFO_SA_FULL_SERVICE == mEvt->new_status)
			{
				threedo_send_message(MSG_ID_THREEDO_TASK_START_REQ, NULL, MOD_MMI, MOD_THREEDO, 0);
			#if defined(__PROJECT_FOR_ZHANGHU__)
				threedo_keep_active();
			#endif
			}
		}
		break;
	case EVT_ID_SRV_SIM_CTRL_AVAILABLE:
		break;
	case EVT_ID_SRV_SIM_CTRL_UNAVAILABLE:
		break;
	case EVT_ID_SRV_SIM_CTRL_REFRESHED:
		break;
	case EVT_ID_SRV_SIM_CTRL_NO_SIM_AVAILABLE:
		break;
	case EVT_ID_SRV_SIM_CTRL_HOME_PLMN_CHANGED:
		break;
	case EVT_ID_SRV_SIM_CTRL_IMSI_CHANGED:
		break;
	case EVT_ID_SRV_SIM_CTRL_EVENT_DETECTED:
		break;

	/* keypad */
	case EVT_ID_POST_KEY:
	case EVT_ID_PRE_KEY_EVT_ROUTING:		
	case EVT_ID_POST_KEY_EVT_ROUTING:
		break;

	/* others */
	case EVT_ID_SRV_CHARBAT_NOTIFY:
	case EVT_ID_SRV_SHUTDOWN_DEINIT:
    case EVT_ID_SRV_SHUTDOWN_NORMAL_START:
    case EVT_ID_SRV_SHUTDOWN_FINAL_DEINIT:
		break;
    case EVT_ID_SRV_DTCNT_ACCT_UPDATE_IND:
		TD_TRACE("[threedo] EVT_ID_SRV_DTCNT_ACCT_UPDATE_IND");
    	break;
	
	default:
		break;
	}

	return ret;
}

