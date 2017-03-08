
#ifndef __THREEDO_PARSE_H__
#define __THREEDO_PARSE_H__

#include "MMI_include.h"
#include "ThreedoGprot.h"

#define TD_KEY_STRING_LENGTH  15 //�����Ϊ15��Ӣ���ַ�
#define TD_VAL_STRING_LENGTH  47 //��ֵ�Ϊ48��Ӣ���ַ�
typedef struct {
	char key[TD_KEY_STRING_LENGTH+1];
	char val[TD_VAL_STRING_LENGTH+1];
}td_kv_map;

typedef enum {
	TD_PARSED_SUCCESS = 0,
	TD_PRASED_ERROR   = -1, //δ֪����
	TD_PARSED_EMPTY   = -2, //����
	TD_PARSED_EXPLAIN = -3, //ע����
	TD_PARSED_NO_VALUE= -4, //û�м�ֵ
	TD_PARSED_NO_EQUAL= -5, //û�з��ָ�ֵ����
	TD_PARSED_KEY_LONG= -6, //����̫��
	TD_PARSED_VAL_LONG= -7, //��ֵ̫��
}td_parse_rslt;


typedef struct {
	char uldomain[THREEDO_DOMIAN_LENGTH+1];
	char dldomain[THREEDO_DOMIAN_LENGTH+1];
	char ulrpath[THREEDO_URL_PARAM_LENGTH+1];
	char dlrpath[THREEDO_URL_PARAM_LENGTH+1];
	char dlsavefd[THREEDO_FILE_NAME_LENGTH+1];
	char apn[THREEDO_APN_NAME_LENGTH+1];
	char smsnum[THREEDO_PHONE_NUM_LENGTH+1];
	char callnum[THREEDO_PHONE_NUM_LENGTH+1];
	U16  ulport;
	U16  dlport;
	U16  heartbeat;
}threedo_conf_info;


extern const char *tdConfKeyUlDomain;
extern const char *tdConfKeyUlParam;
extern const char *tdConfKeyUlPort;

extern const char *tdConfKeyDlDomain;
extern const char *tdConfKeyDlParam;
extern const char *tdConfKeyDlPort;
extern const char *tdConfKeyDlSavePath;

extern const char *tdConfKeyCallNum;
extern const char *tdConfKeySmsNum;
extern const char *tdConfKeyHeartbeat;
extern const char *tdConfKeyApn;


extern void threedo_get_default_config_info(threedo_conf_info *out);
extern S32 threedo_write_config_info(threedo_conf_info *info);
extern S32 threedo_read_config_info(threedo_conf_info *out);

#if defined(__THREEDO_TEST_CODE__)
extern void threedo_file_parse_test(void);
#endif

#endif/*__THREEDO_PARSE_H__*/


