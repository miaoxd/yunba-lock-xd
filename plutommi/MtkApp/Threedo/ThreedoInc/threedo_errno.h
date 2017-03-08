
#ifndef __THREEDO_ERRNO_H__
#define __THREEDO_ERRNO_H__

#define TD_ERRNO_NO_ERROR                   (0)

/* gernanal errorS */
#define TD_ERRNO_UNKOWN_ERR                 (-1)
#define TD_ERRNO_PARAM_ILLEGAL              (-2)
#define TD_ERRNO_MALLOC_FAILED              (-3)
#define TD_ERRNO_EMPTY_PORINTER             (-4)
#define TD_ERRNO_BUFFER_TOO_SHORT           (-5)
#define TD_ERRNO_VALUE_OUT_OF_RANGE         (-6)
#define TD_ERRNO_MODULE_BUSING              (-7)
#define TD_ERRNO_GENERIC_NAME_FAILE         (-8)
#define TD_ERRNO_LENGTH_IS_ZERO             (-9)
#define TD_ERRNO_UNDEFINED_ACTION           (-10)
#define TD_ERRNO_WOULDBLOCK                   (-11)

/* FS errors */
#define TD_ERRNO_PATH_NOT_EXIST             (-101)
#define TD_ERRNO_FILE_NOT_FOUND             (-102)
#define TD_ERRNO_FILE_CHECK_ERR             (-103)
#define TD_ERRNO_FILE_OPEN_FAILED           (-104)

/* disk errors */
#define TD_ERRNO_CARD_NOT_EXIST             (-201)
#define TD_ERRNO_CARD_NOT_USABLE            (-202)
#define TD_ERRNO_CARD_MEM_FULL              (-203)
#define TD_ERRNO_CARD_NO_SPACE              (-204)
#define TD_ERRNO_CARD_BUSING                (-205)
#define TD_ERRNO_CARD_IN_USB_MODE           (-206)

/* SIM errors */
#define TD_ERRNO_SIM_NOT_READY              (-301)

/* network errors */
#define TD_ERRNO_NW_NOT_USABLE              (-401)

/* socket errors */
#define TD_ERRNO_ACCOUNT_ERR                (-501)
#define TD_ERRNO_APPID_FAILED               (-502)
#define TD_ERRNO_SOCKET_FAILED              (-503)
#define TD_ERRNO_SOCKET_SPLITING            (-504)
#define TD_ERRNO_SOC_CREATE_FAIL            (-505)
#define TD_ERRNO_SOC_SET_ASYNC              (-506)
#define TD_ERRNO_SOC_SET_NBIO               (-507)

/* HTTP head parse */
#define TD_ERRNO_HTTP_ERROR_CODE            (-601)
#define TD_ERRNO_HTTP_FIND_CONTENT_TAG      (-602)
#define TD_ERRNO_HTTP_CONTENT_LENGTH        (-602)
#define TD_ERRNO_HTTP_CONTENT_POSITION      (-603)

/* media errors */
#define TD_ERRNO_FINISH_LIMITED_TIME        (-701)
#define TD_ERRNO_FINISH_LIMITED_SIZE        (-702)
#define TD_ERRNO_ADOREC_FAILED              (-703)
#define TD_ERRNO_ADOPLY_FAILED              (-704)
#define TD_ERRNO_ADOREC_START_FILED         (-705)
#define TD_ERRNO_ADOREC_STOP_FILED          (-706)

#endif

