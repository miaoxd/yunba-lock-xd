
#ifndef __THREEDO_GPROT_H__
#define __THREEDO_GPROT_H__

typedef void (*THREEDO_NCBF)(void*);
typedef U8 (*THREEDO_MSG_CBF)(void*);

#define TD_BOOL         int
#define TD_FALSE        0
#define TD_TRUE         1

//Ĭ��ֵ
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

#define THREEDO_HEEARBEAT        80  //80��һ������

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

#define THREEDO_HEEARBEAT        300  //300��һ������

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
** ����: threedo_create_path
** ����: ����һ����ȵ�·��
** ����: drive   -- �̷�
**       folder  -- �ļ�������
** ����: �Ƿ񴴽��ɹ�
** ����: threedo
*******/
extern S32 threedo_create_path(S32 drive, const WCHAR *folder);

/*******************************************************************************
** ����: threedo_create_path_multi
** ����: ����·��, �����Ƕ���·��
** ����: dirve     -- �̷�
**       UcsFolder -- �ļ���
** ����: ��
** ����: threedo
*******/
extern S32 threedo_create_path_multi(S32 drive, const WCHAR *UcsFolder);

/*******************************************************************************
** ����: threedo_create_path_multi_ex
** ����: ����·��, �����Ƕ���·��
** ����: path ·��
** ����: ��
** ����: threedo
*******/
extern S32 threedo_create_path_multi_ex(const WCHAR *path);


extern S32 threedo_log(const char *fmt, ...);

//�������ڴ�
extern void *threedo_malloc(U32 size);
extern void threedo_mfree(void *ptr);
//����С���ڴ�(2K����)
extern void *threedo_smalloc(U16 size);
extern void threedo_smfree(void *ptr);

//������Ϣ����ͬ��ģ��
extern void threedo_send_message(U16 msg_id, void *req, int mod_src, int mod_dst, int sap);

/*******************************************************************************
** ����: threedo_send_message_to_mmi
** ����: ������Ϣ��MMIģ��
** ����: msgid  -- ��ϢID
**       msg    -- ��Ϣ
** ����: ��
** ����: threedo
*******/
extern void threedo_send_message_to_mmi(msg_type msgid, void *msg);

/*******************************************************************************
** ����: threedo_construct_para
** ����: ��������buffer
** ����: sz   ��ʾ��Ҫʹ�õ�buffer��С.
** ����: ��
** ����: threedo
*******/
extern void *threedo_construct_para(U32 sz);

/*******************************************************************************
** ����: threedo_destruct_para
** ����: ����buffer�ͷ�
** ����: ptr   ��ʾ��Ҫ�ͷŵĲ���buffer��ַ.
** ����: ��
** ����: threedo
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
** ����: threedo_get_system_tick
** ����: ��ȡϵͳTICK
** ����: ��
** ����: ��
** ����: threedo
*******/
extern U32 threedo_get_system_tick(void);

/*******************************************************************************
** ����: threedo_get_current_time
** ����: ��ȡϵͳ��ǰ������ʱ��
** ����: ��
** ����: ����ʱ��
** ����: threedo
*******/
extern U32 threedo_get_current_time(void);

/*******************************************************************************
** ����: threedo_get_date_time
** ����: ��ȡϵͳ��ǰ��ʱ��
** ����: ʱ��ṹ��
** ����: ��
** ����: threedo
*******/
extern void threedo_get_date_time(applib_time_struct *t);

/*******************************************************************************
** ����: threedo_set_date_time
** ����: ����ϵͳ��ǰ��ʱ��
** ����: ʱ��ṹ��
** ����: ��
** ����: threedo
*******/
extern void threedo_set_date_time(applib_time_struct *t);

/*******************************************************************************
** ����: threedo_is_sim_usable
** ����: �ж�SIM���Ƿ����ʹ��, ����SIM����ȡ������У�鶼ͨ���˲������ʹ��.
** ����: sim  -- SIM������
** ����: �Ƿ����ʹ��
** ����: threedo
*******/
extern S32 threedo_is_sim_usable(mmi_sim_enum sim);

/*******************************************************************************
** ����: threedo_get_usable_drive
** ����: ��ȡϵͳ���õĴ洢�豸�̷�
** ����: ��
** ����: �����̷� 0x43~0x47
** ����: threedo
*******/
extern S32 threedo_get_usable_drive(void);

//��ȡT���̷�
extern S32 threedo_is_tcard_exist(void);
extern S32 threedo_is_drive_usable(S32 drive);
extern U32 threedo_get_disk_free_size(S32 drive);
extern S32 threedo_get_card_drive(void);
//��ȡ�ֻ�U���̷�
extern S32 threedo_get_public_drive(void);
//��ȡϵͳ���̷�
extern S32 threedo_get_private_drive(void);
//ע�ᡢע����Ϣ�ص�����
extern void threedo_clear_message_protocol(U16 msgid, THREEDO_MSG_CBF funcPtr);
extern void threedo_set_message_protocol(U16 msgid, THREEDO_MSG_CBF funcPtr);
extern void threedo_set_message_protocol_multi(U16 msgid, THREEDO_MSG_CBF funcPtr);

extern WCHAR *threedo_find_file_name_position(WCHAR *path);
extern WCHAR *threedo_find_file_suffix_name_position(WCHAR *path);

/*******************************************************************************
** ����: threedo_set_data_account
** ����: ����GPRS�����˻���Ϣ����
** ����: acc_info -- �����˻������ݽṹ srv_dtcnt_prof_gprs_struct.
** ����: ��.
** ˵��: prof->prof_fields �μ� dtcntsrviprot.h �е� SRV_DTCNT_PROF_FIELD_xxx.
** ����: threedo
*******/
//extern S32 threedo_set_data_account(srv_dtcnt_prof_gprs_struct *acc_info);

//��绰
extern S32 threedo_make_call(const char *number, S32 simid);
//������
extern S32 threedo_send_sms(const WCHAR *ucs_text, const char *number, S32 simid);
//��������
extern S32 threedo_play_audio(const WCHAR *ado_path);

#endif/*__THREEDO_GPROT_H__*/

