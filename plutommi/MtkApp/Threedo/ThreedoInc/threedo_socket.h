
#ifndef __THREEDO_SOCKET_H__
#define __THREEDO_SOCKET_H__

#include "MMIDataType.h"

#include "soc_api.h"
#include "soc_consts.h"
#include "cbm_consts.h"
#include "DtcntSrvIntStruct.h"
#include "DtcntSrvDb.h"
#include "app2soc_struct.h"


#define THREEDO_INVALID_SOCID    (-1)

#define TD_SOC_TYPE_SIM1_TCP     (0x00)
#define TD_SOC_TYPE_SIM1_UDP     (0x01)
#define TD_SOC_TYPE_SIM2_TCP     (0x10)
#define TD_SOC_TYPE_SIM2_UDP     (0x11)
#define TD_SOC_TYPE_WLAN_TCP     (0xF0)
#define TD_SOC_TYPE_WLAN_UDP     (0xF1)

typedef struct
{
    MMI_EVT_PARAM_HEADER

    app_soc_get_host_by_name_ind_struct domain;
	app_soc_notify_ind_struct           notify;
}threedo_socket_event_struct;

typedef enum {
	THREEDO_DNS_IGNORE  = 0x00,
	THREEDO_DNS_START   = 0x01,
	THREEDO_DNS_BLOCKED = 0x02,
	THREEDO_DNS_FAILED  = 0x04,
	THREEDO_DNS_SUCCESS = 0x08,
	
	THREEDO_DNS_FINISHED = 0x80,
}threedo_dns_state;

typedef struct _td_soc_mngr {
	sockaddr_struct addr;
	S32  error;
	S32  req_id;
	S32  soc_id; //总是存在数据类型转换的问题，mbd
	S8   get_ip; //是否正在解析域名, threedo_dns_state
}THRD_SOC_MNGR;

typedef struct {
	void *content;
	S32   errorCode; //HTTP/1.1 200 OK
	S32   headLength;
	S32   contentLength; //Content-Length: 5094
	S8    keep_alive;    //Connection: keep-alive
	S8    content_type;  //Content-Type: application/octet-stream
}HTTP_RSP_HEAD;

extern const char *http_head_line_end;
extern const char *http_head_body_split;
extern const char *http_version;
extern const char *http_content_length;
extern const char *http_content_type;
extern const char *http_connection;
extern const char *http_content_dispositon;
extern const char *http_form_boundary;
extern const char *http_form_data;

#define THREEDO_HTTP_NOT_FOUND_TAG             -1

#define THREEDO_HTTP_RETVAL_OK                 200
#define THREEDO_HTTP_RETVAL_NOT_FOUND          404
#define THREEDO_HTTP_RETVAL_SRV_ERROR          500

#define THREEDO_HTTTP_CONTENT_TYPE_IS_OCTET_STREAM     1
#define THREEDO_HTTTP_CONTENT_TYPE_IS_JSON             2

extern S32 threedo_socket_allocate_app_id(U8 *idout);
extern void threedo_socket_free_app_id(U8 id);
extern U32 threedo_socket_get_account_id(char *apn, U8 appid, U32 flags);

extern S32 threedo_socket_open(
	THRD_SOC_MNGR *pSoc,
	char *addr,
	S32 mod, //MOD_MMI
	U32 accid,
	S32 type,//socket_type_enum
	U16 port);

extern void threedo_socket_close(THRD_SOC_MNGR *ppSoc);
extern S32 threedo_socket_connect(THRD_SOC_MNGR *pSoc);
extern S32 threedo_socket_write(THRD_SOC_MNGR *pSoc, void *buffer, unsigned int length);
extern S32 threedo_socket_read(THRD_SOC_MNGR *pSoc, void *buffer, unsigned int length);
extern S32 threedo_socket_parse_http_head(HTTP_RSP_HEAD *out, const char *dt, U32 length);

#endif/*__THREEDO_SOCKET_H__*/

