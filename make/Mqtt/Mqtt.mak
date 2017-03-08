# Define source file lists to SRC_LIST

MQTT_PATH=plutommi\MtkApp\Mqtt

SRC_LIST = $(strip $(MQTT_PATH))\mqtt_demo.c \
           $(strip $(MQTT_PATH))\MQTTPacket\src\MQTTConnectClient.c \
           $(strip $(MQTT_PATH))\MQTTPacket\src\MQTTDeserializePublish.c \
           $(strip $(MQTT_PATH))\MQTTPacket\src\MQTTFormat.c \
           $(strip $(MQTT_PATH))\MQTTPacket\src\MQTTSerializePublish.c \
           $(strip $(MQTT_PATH))\MQTTPacket\src\MQTTSubscribeClient.c \
           $(strip $(MQTT_PATH))\MQTTPacket\src\MQTTUnsubscribeClient.c \
           $(strip $(MQTT_PATH))\MQTTPacket\src\MQTTConnectServer.c \
           $(strip $(MQTT_PATH))\MQTTPacket\src\MQTTDeserializePublish2.c \
           $(strip $(MQTT_PATH))\MQTTPacket\src\MQTTPacket.c \
           $(strip $(MQTT_PATH))\MQTTPacket\src\MQTTSerializePublish2.c \
           $(strip $(MQTT_PATH))\MQTTPacket\src\MQTTSubscribeServer.c \
           $(strip $(MQTT_PATH))\MQTTPacket\src\MQTTUnsubscribeServer.c \
           $(strip $(MQTT_PATH))\MQTTClient-C\src\MQTTClient.c \
           $(strip $(MQTT_PATH))\MQTTClient-C\src\mtk\mtk_socket.c \
           $(strip $(MQTT_PATH))\MQTTClient-C\src\mtk\mqttmtk.c

#  Define include path lists to INC_DIR
INC_DIR = $(strip $(MQTT_PATH))\MQTTPacket\src \
          $(strip $(MQTT_PATH))\MQTTClient-C\src \
          $(strip $(MQTT_PATH))\MQTTClient-C\src\mtk
 
# Define the specified compile options to COMP_DEFS
COMP_DEFS = __PROJECT_FOR_ZHANGHU__


ifneq ($(filter __PROJECT_FOR_ZHANGHU__,$(COMP_DEFS)),)
    # SRC_LIST +=  $(strip $(MQTT_PATH))\ThreedoSrc\threedo_yunba.c
endif
