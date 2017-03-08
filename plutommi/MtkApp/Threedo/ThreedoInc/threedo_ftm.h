
#ifndef __THREEDO_FTM_H__
#define __THREEDO_FTM_H__

#include "kal_public_defs.h"
#include "threedo_socket.h"
#include "app2soc_struct.h"
#include "threedo_parse.h"

#if !defined(__THREEDO_HTTP_KEEP_ALIVE__)
#define __THREEDO_HTTP_KEEP_ALIVE__
#endif


#define TD_FTTM_PATH_DEEPTH            63
#define TD_FTTM_SUFFIX_NAME_LEN        5
#define TD_FTTM_URL_LENGTH             255
#if defined(__PROJECT_FOR_ZHANGHU__)
#define TD_FTTM_SOC_BUF_SIZE           1536
#define TD_FTTM_HTTP_TAIL_BUF_SIZE     512
#else
#define TD_FTTM_SOC_BUF_SIZE           2048
#endif

#define TD_FTTM_TASK_MAX               6
#define TD_FTTM_RETRY_MAX              20
#define TD_FTTM_TAKE_TICK_MAX          (1000*10*8)  //һ��TICK��Լ��4ms

#define TD_FTTM_SOC_BLOCK_TIME         6000 //����Ϊ��λ
#define TD_FTTM_SOC_KEEP_TIME          3000  //����Ϊ��λ
#define TD_FTTM_WAIT_RECV_END_TIME     3000  //����Ϊ��λ


//����Ĵ���״̬
#define TD_TT_ACTION_IS_UNKOWN       0x00
#define TD_TT_ACTION_IS_UPLOAD       0x01
#define TD_TT_ACTION_IS_DOWNLOAD     0x02
#define TD_TT_ACTION_IS_HEARTBEAT    0x03

//�ļ�/���ݴ����״̬
#define TD_TT_WAIT_START       0x00
#define TD_TT_READY_SOCKET     0x01
#define TD_TT_HTTP_POST_SENT   0x02
#define TD_TT_HTTP_GET_SENT    0x03
#define TD_TT_WAIT_SRV_ACK     0x04
#define TD_TT_UPLOAD_CONTI     0x05
#define TD_TT_DOWNLOAD_CONTI   0x06
#define TD_TT_FD_TRANS_EOF     0x07
#define TD_TT_TRANS_FINISH     0x08
#define TD_TT_UPLOAD_TAIL      0x09

//�ļ�������
#define TD_TT_TYPE_AMR         0x00
#define TD_TT_TYPE_MP3         0x01
#define TD_TT_TYPE_WAV         0x02
#define TD_TT_TYPE_MID         0x03
#define TD_TT_TYPE_HRTBT       0x04

typedef struct td_tt_node{
	U8     path[sizeof(WCHAR)*(TD_FTTM_PATH_DEEPTH+1)];  //�ļ�/������Դ�ľ�����λ��: URL���ߴ���·��
	char   domain[THREEDO_DOMIAN_LENGTH+1];
	char   urlParam[THREEDO_URL_PARAM_LENGTH+1];
	
	S32    fsHandle;    //�ļ�/���ݵĴ洢���
	U32    fileSize;    //�ļ�/���ݵ��ܴ�С, BYTE
	U32    filePoint;   //�ļ�/���ݵĵ�ǰ��ȡ/д��λ��
	U32    currSize;    //�ļ�/���ݵ��Ѷ�ȡ/д��SOCKET�Ĵ�С
	
	U32    insertTick;  //���������е�ʱ���, ���ʱ��̫���ˣ����ܿ��Ǹɵ���
	U16    port;
	U8     retryCt;	    //��ǰ�������ִ�д���
	U8     action;      //��ǰ�������ϴ���������
	U8     state;       //��ǰ�����״̬: �ȴ�������/���ڴ���/�ȴ������ź�
	U8     type;        //��ǰ�ļ�/���ݵ�����(�ļ���)
	struct td_tt_node      *next;
}threedo_tt_node;


typedef struct {
	char     domain[THREEDO_DOMIAN_LENGTH+1];
	soc_dns_a_struct entry[SOC_MAX_A_ENTRY]; /* entries */
    kal_uint8	addr[16];   /* for backward compatibility */
	U32         ticks;      /* record time */
    kal_bool    result;     /* the result of soc_gethostbyname */
    kal_uint8   num_entry;
    kal_uint8	addr_len;   /* the first record in entry,  */
}trheedo_dns_ret;

/* �ļ������������ */
typedef struct {
	U8    buffer[TD_FTTM_SOC_BUF_SIZE];
#if defined(__PROJECT_FOR_ZHANGHU__)
	U8    tail[TD_FTTM_HTTP_TAIL_BUF_SIZE];
#endif
	THRD_SOC_MNGR    sock;
	threedo_tt_node *task;
	threedo_tt_node  tQueue[TD_FTTM_TASK_MAX];
#if defined(__THREEDO_USE_LAST_DNS_RESULT__)
	trheedo_dns_ret  dns;
#endif
	threedo_conf_info conf;
	U32 length;
	U32 ticks; //���һ�����ݽ������ʱ��tick
	U32 acc_id; //��¼����, �Ա�����
#if defined(__THREEDO_HTTP_KEEP_ALIVE__)
	S8  keep_http_alive;
	U8  last_upload_read;
#endif
	U8  app_id;
}threedo_fttm;

typedef struct {
	LOCAL_PARA_HDR 
	WCHAR path[TD_FTTM_PATH_DEEPTH+1];
	U32   size;
	U8    type;
	U8    action;
}threedo_ft_proc_req;

typedef struct {
	LOCAL_PARA_HDR 
	U16   timer_id;
}threedo_ft_timeout_ind;

typedef struct {
	LOCAL_PARA_HDR 
	WCHAR path[TD_FTTM_PATH_DEEPTH+1];
	S32   error;
	U32   size;
	U8    action;
	U8    type;
}threedo_ft_finish_ind;

/* �� MOD_THREEDO ģ����� */
extern void threedo_file_trans_check_task_queue(void);
extern void threedo_file_trans_process_ready(threedo_ft_proc_req *arg);
extern void threedo_file_trans_process(void);
extern S32 threedo_file_trans_socket_event_handle(ilm_struct *ilm);
extern void threedo_file_trans_timeout_proc(void *v);
extern void threedo_file_trans_init(void);
extern void threedo_file_trans_free_all(void);

/* ������ģ����� */
extern void threedo_send_file_trans_req(const WCHAR *path, U32 size, U8 type, U8 act);

extern void threedo_download_start(WCHAR *filename);

#if defined(__PROJECT_FOR_ZHANGHU__)
extern void threedo_keep_active(void);
#endif


#endif/*__THREEDO_FTM_H__*/

