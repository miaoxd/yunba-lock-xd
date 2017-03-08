# Define source file lists to SRC_LIST

THREEDO_PATH=plutommi\MtkApp\Threedo

SRC_LIST = $(strip $(THREEDO_PATH))\ThreedoSrc\threedo_crc.c \
           $(strip $(THREEDO_PATH))\ThreedoSrc\threedo_encrypt.c \
           $(strip $(THREEDO_PATH))\ThreedoSrc\threedo_ftm.c \
		   $(strip $(THREEDO_PATH))\ThreedoSrc\threedo_sdtm.c \
           $(strip $(THREEDO_PATH))\ThreedoSrc\threedo_media.c \
           $(strip $(THREEDO_PATH))\ThreedoSrc\threedo_socket.c \
           $(strip $(THREEDO_PATH))\ThreedoSrc\threedo_win32.c \
           $(strip $(THREEDO_PATH))\ThreedoSrc\threedo_parse.c \
           $(strip $(THREEDO_PATH))\ThreedoSrc\threedo_window.c \
           $(strip $(THREEDO_PATH))\ThreedoSrc\threedo_task.c

#  Define include path lists to INC_DIR
INC_DIR = $(strip $(THREEDO_PATH))\ThreedoInc
 
# Define the specified compile options to COMP_DEFS
COMP_DEFS = __PROJECT_FOR_ZHANGHU__


ifneq ($(filter __PROJECT_FOR_ZHANGHU__,$(COMP_DEFS)),)
    SRC_LIST +=  $(strip $(THREEDO_PATH))\ThreedoSrc\threedo_yunba.c
endif

