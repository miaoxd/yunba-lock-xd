
#ifndef __THREEDO_GPROT_H__
#define __THREEDO_GPROT_H__

typedef void (*THREEDO_NCBF)(void*);
typedef U8 (*THREEDO_MSG_CBF)(void*);

#define TD_BOOL         int
#define TD_FALSE        0
#define TD_TRUE         1

//默认值
#if defined(__PROJECT_FOR_ZHANGHU__)

#define THREEDO_FOLDER           "_Zhanghu"
#define THREEDO_CONFIG_FILE_NAME "zhangh.conf"
//download
//#define THREEDO_DL_DOMAIN        "f2.yunba.io"
#define THREEDO_DL_DOMAIN        "58.254.170.3"
#define THREEDO_DL_RPATH         "/files"
#define THREEDO_DL_PORT          8888
#define THREEDO_DL_SAVE_FOLDER   "dlAdo"
//upload
#define THREEDO_UL_DOMAIN        "58.254.170.3"
//#define THREEDO_UL_DOMAIN        "f2.yunba.io"
#define THREEDO_UL_RPATH         "/upload"
#define THREEDO_UL_PORT          8888

#define THREEDO_HEEARBEAT        80  //80秒一次心跳

#define THREEDO_CALL_NUMBER      "012345"
#define THREEDO_SMS_NUMBER       "012345"

#define THREEDO_DEFAULT_APN      "cmnet"
#else

#define THREEDO_FOLDER           "threedo"
#define THREEDO_CONFIG_FILE_NAME "threedo.conf"
//download
#define THREEDO_DL_DOMAIN        "127.0.0.1"
#define THREEDO_DL_RPATH         "/files"
#define THREEDO_DL_PORT          20000
#define THREEDO_DL_SAVE_FOLDER   "download"
//upload
#define THREEDO_UL_DOMAIN        THREEDO_DL_DOMAIN
#define THREEDO_UL_RPATH         THREEDO_DL_RPATH
#define THREEDO_UL_PORT          THREEDO_DL_PORT

#define THREEDO_HEEARBEAT        300  //300秒一次心跳

#define THREEDO_CALL_NUMBER      "13812345678"
#define THREEDO_SMS_NUMBER       "13812345678"

#define THREEDO_DEFAULT_APN      "internet"
#endif

#define THREEDO_PATH_DEEPTH      127

#define THREEDO_FILE_LINE_LENGTH 127

// files.domain_sample.com:domain_port/url_parameters_followed
#define THREEDO_URL_LENGTH       127
#define THREEDO_DOMIAN_LENGTH    31
#define THREEDO_URL_PARAM_LENGTH (THREEDO_URL_LENGTH-THREEDO_DOMIAN_LENGTH-1)
#define THREEDO_APN_NAME_LENGTH  31
#define THREEDO_USERNAEM_LENGTH  31
#define THREEDO_PASSWORD_LENGTH  31
#define THREEDO_IP_STRING_LENGTH 15
#define THREEDO_PHONE_NUM_LENGTH 21
#define THREEDO_FILE_NAME_LENGTH 31

#define TD_ASSERT(v)    MMI_ASSERT(v)
#if defined(WIN32)
#define TD_TRACE(f, ...) do {printf(f, ##__VA_ARGS__);printf("\r\n");\
							threedo_log(f, ##__VA_ARGS__);\
						}while(0)
#else
#define TD_TRACE(f, ...) do {kal_prompt_trace(MOD_THREEDO, f, ##__VA_ARGS__);\
						}while(0)
#endif

#define TD_GET_TICK()    kal_get_systicks()

/*******************************************************************************
** 函数: threedo_create_path
** 功能: 创建一级深度的路径
** 参数: drive   -- 盘符
**       folder  -- 文件夹名字
** 返回: 是否创建成功
** 作者: threedo
*******/
extern S32 threedo_create_path(S32 drive, const WCHAR *folder);

/*******************************************************************************
** 函数: threedo_create_path_multi
** 功能: 创建路径, 可以是多重路径
** 参数: dirve     -- 盘符
**       UcsFolder -- 文件夹
** 返回: 无
** 作者: threedo
*******/
extern S32 threedo_create_path_multi(S32 drive, const WCHAR *UcsFolder);

/*******************************************************************************
** 函数: threedo_create_path_multi_ex
** 功能: 创建路径, 可以是多重路径
** 参数: path 路径
** 返回: 无
** 作者: threedo
*******/
extern S32 threedo_create_path_multi_ex(const WCHAR *path);


extern S32 threedo_log(const char *fmt, ...);

//申请大块内存
extern void *threedo_malloc(U32 size);
extern void threedo_mfree(void *ptr);
//申请小块内存(2K以内)
extern void *threedo_smalloc(U16 size);
extern void threedo_smfree(void *ptr);

//发送消息到不同的模块
extern void threedo_send_message(U16 msg_id, void *req, int mod_src, int mod_dst, int sap);

/*******************************************************************************
** 函数: threedo_send_message_to_mmi
** 功能: 发送消息到MMI模块
** 参数: msgid  -- 消息ID
**       msg    -- 消息
** 返回: 无
** 作者: threedo
*******/
extern void threedo_send_message_to_mmi(msg_type msgid, void *msg);

/*******************************************************************************
** 函数: threedo_construct_para
** 功能: 参数构造buffer
** 参数: sz   表示需要使用的buffer大小.
** 返回: 无
** 作者: threedo
*******/
extern void *threedo_construct_para(U32 sz);

/*******************************************************************************
** 函数: threedo_destruct_para
** 功能: 参数buffer释放
** 参数: ptr   表示需要释放的参数buffer地址.
** 返回: 无
** 作者: threedo
*******/
extern void threedo_destruct_para(void *ptr);

extern void threedo_md5(char *out, U8 *data, U32 length);
extern void threedo_md5_file(char *out, const WCHAR *file);

extern void threedo_base64(char *out, U32 out_length, U8 *src, U32 src_len);
extern void threedo_base64_file(char *out, U32 out_length, const WCHAR *file);

extern U32 threedo_crc32(U8 *data, U32 length);
extern U32 threedo_crc32_file(const WCHAR *file);

extern WCHAR *threedo_get_file_name_from_path(WCHAR *path);

extern S32 threedo_get_usable_sim_index(void);

/*******************************************************************************
** 函数: threedo_get_system_tick
** 功能: 获取系统TICK
** 参数: 无
** 返回: 无
** 作者: threedo
*******/
extern U32 threedo_get_system_tick(void);

/*******************************************************************************
** 函数: threedo_get_current_time
** 功能: 获取系统当前的运行时间
** 参数: 无
** 返回: 运行时间
** 作者: threedo
*******/
extern U32 threedo_get_current_time(void);

/*******************************************************************************
** 函数: threedo_get_date_time
** 功能: 获取系统当前的时间
** 参数: 时间结构体
** 返回: 无
** 作者: threedo
*******/
extern void threedo_get_date_time(applib_time_struct *t);

/*******************************************************************************
** 函数: threedo_set_date_time
** 功能: 设置系统当前的时间
** 参数: 时间结构体
** 返回: 无
** 作者: threedo
*******/
extern void threedo_set_date_time(applib_time_struct *t);

/*******************************************************************************
** 函数: threedo_is_sim_usable
** 功能: 判断SIM卡是否可以使用, 仅当SIM卡读取正常且校验都通过了才算可以使用.
** 参数: sim  -- SIM卡索引
** 返回: 是否可以使用
** 作者: threedo
*******/
extern S32 threedo_is_sim_usable(mmi_sim_enum sim);

/*******************************************************************************
** 函数: threedo_get_usable_drive
** 功能: 获取系统可用的存储设备盘符
** 参数: 无
** 返回: 返回盘符 0x43~0x47
** 作者: threedo
*******/
extern S32 threedo_get_usable_drive(void);

//获取T卡盘符
extern S32 threedo_is_tcard_exist(void);
extern S32 threedo_is_drive_usable(S32 drive);
extern U32 threedo_get_disk_free_size(S32 drive);
extern S32 threedo_get_card_drive(void);
//获取手机U盘盘符
extern S32 threedo_get_public_drive(void);
//获取系统盘盘符
extern S32 threedo_get_private_drive(void);
//注册、注销消息回调函数
extern void threedo_clear_message_protocol(U16 msgid, THREEDO_MSG_CBF funcPtr);
extern void threedo_set_message_protocol(U16 msgid, THREEDO_MSG_CBF funcPtr);
extern void threedo_set_message_protocol_multi(U16 msgid, THREEDO_MSG_CBF funcPtr);

extern WCHAR *threedo_find_file_name_position(WCHAR *path);
extern WCHAR *threedo_find_file_suffix_name_position(WCHAR *path);

/*******************************************************************************
** 函数: threedo_set_data_account
** 功能: 设置GPRS数据账户信息请求
** 参数: acc_info -- 数据账户的数据结构 srv_dtcnt_prof_gprs_struct.
** 返回: 无.
** 说明: prof->prof_fields 参见 dtcntsrviprot.h 中的 SRV_DTCNT_PROF_FIELD_xxx.
** 作者: threedo
*******/
//extern S32 threedo_set_data_account(srv_dtcnt_prof_gprs_struct *acc_info);

//打电话
extern S32 threedo_make_call(const char *number, S32 simid);
//发短信
extern S32 threedo_send_sms(const WCHAR *ucs_text, const char *number, S32 simid);
//播放音乐
extern S32 threedo_play_audio(const WCHAR *ado_path);

#endif/*__THREEDO_GPROT_H__*/

