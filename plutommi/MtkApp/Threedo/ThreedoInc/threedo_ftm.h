
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
#define TD_FTTM_TAKE_TICK_MAX          (1000*10*8)  //一个TICK大约在4ms

#define TD_FTTM_SOC_BLOCK_TIME         6000 //毫秒为单位
#define TD_FTTM_SOC_KEEP_TIME          3000  //毫秒为单位
#define TD_FTTM_WAIT_RECV_END_TIME     3000  //毫秒为单位


//任务的处理状态
#define TD_TT_ACTION_IS_UNKOWN       0x00
#define TD_TT_ACTION_IS_UPLOAD       0x01
#define TD_TT_ACTION_IS_DOWNLOAD     0x02
#define TD_TT_ACTION_IS_HEARTBEAT    0x03

//文件/数据处理的状态
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

//文件的类型
#define TD_TT_TYPE_AMR         0x00
#define TD_TT_TYPE_MP3         0x01
#define TD_TT_TYPE_WAV         0x02
#define TD_TT_TYPE_MID         0x03
#define TD_TT_TYPE_HRTBT       0x04

typedef struct td_tt_node{
	U8     path[sizeof(WCHAR)*(TD_FTTM_PATH_DEEPTH+1)];  //文件/数据资源的具体存放位置: URL或者磁盘路径
	char   domain[THREEDO_DOMIAN_LENGTH+1];
	char   urlParam[THREEDO_URL_PARAM_LENGTH+1];
	
	S32    fsHandle;    //文件/数据的存储句柄
	U32    fileSize;    //文件/数据的总大小, BYTE
	U32    filePoint;   //文件/数据的当前读取/写入位置
	U32    currSize;    //文件/数据的已读取/写入SOCKET的大小
	
	U32    insertTick;  //任务插入队列的时间点, 如果时间太长了，可能考虑干掉它
	U16    port;
	U8     retryCt;	    //当前任务的已执行次数
	U8     action;      //当前任务是上传还是下载
	U8     state;       //当前任务的状态: 等待被处理/正在传输/等待结束信号
	U8     type;        //当前文件/数据的类型(文件名)
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

/* 文件传输任务管理 */
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
	U32 ticks; //最后一次数据交互完成时的tick
	U32 acc_id; //记录下来, 以备后用
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

/* 给 MOD_THREEDO 模块调用 */
extern void threedo_file_trans_check_task_queue(void);
extern void threedo_file_trans_process_ready(threedo_ft_proc_req *arg);
extern void threedo_file_trans_process(void);
extern S32 threedo_file_trans_socket_event_handle(ilm_struct *ilm);
extern void threedo_file_trans_timeout_proc(void *v);
extern void threedo_file_trans_init(void);
extern void threedo_file_trans_free_all(void);

/* 给其他模块调用 */
extern void threedo_send_file_trans_req(const WCHAR *path, U32 size, U8 type, U8 act);

extern void threedo_download_start(WCHAR *filename);

#if defined(__PROJECT_FOR_ZHANGHU__)
extern void threedo_keep_active(void);
#endif


#endif/*__THREEDO_FTM_H__*/

