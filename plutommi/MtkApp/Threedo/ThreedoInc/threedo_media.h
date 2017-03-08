
#ifndef __THREEDO_MEDIA_H__
#define __THREEDO_MEDIA_H__

#include "kal_public_defs.h"

#define THREEDO_AUDIO_FOLER     "ADO"
#define THREEDO_IMAGE_FOLER     "IMG"
#define THREEDO_VIDEO_FOLER     "VDO"

#if defined(__PROJECT_FOR_ZHANGHU__)
#define THREEDO_DOWNLOAD_FOLER   "dlAdo"
#else
#define THREEDO_DOWNLOAD_FOLER   "dl"
#endif

enum td_ado_quality {
	THREEDO_ADO_QUALITY_LOW = 0, //amr
	THREEDO_ADO_QUALITY_HIGH,    //wav
};
#define THREEDO_ADO_QUALITY_LEVEL  0

#define THREEDO_ADO_LIMITED_SEC    20
#define THREEDO_ADO_LIMITED_KB     50

#define SRIC_STATE_IDLE            0
#define SRIC_STATE_RECORDING       1
#define SRIC_STATE_PLAYING         2

typedef struct {
	LOCAL_PARA_HDR
	WCHAR path[THREEDO_PATH_DEEPTH+1];
	void (*callback)(const WCHAR *abs_path, const WCHAR *fname);
}threedo_ado_play_req;


struct sric_context {
    WCHAR  filepath[THREEDO_PATH_DEEPTH+1];
	WCHAR *fileNamePostionIn_filepath;  //这个指针指向filepath里面文件名开始的位置
	int    status;
	U32    time;
	void  (*callback)(const WCHAR *abs_path, const WCHAR *fname);
};

extern S32 threedo_audio_record(U32 timeMs, U32 sizeKb, S32 drive, void (*callback)(const WCHAR *abs_path, const WCHAR *fname));
extern S32 threedo_audio_record_stop(void);

extern S32 threedo_audio_play(const WCHAR *path, void (*callback)(const WCHAR *abs_path, const WCHAR *fname));
extern S32 threedo_audio_play_stop(void);

extern void threedo_audio_play_request(WCHAR *path);

extern void threedo_media_initialize(void);

extern void threedo_custom_audio_record_stop(void);
extern S32 threedo_custom_audio_record_start(void);

extern const char *threedo_media_get_state_string(void);

#endif/*__THREEDO_MEDIA_H__*/

