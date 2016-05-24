#include <c_types.h>
#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include <mem.h>

#include "main.h"
#include "config.h"
#include "uart.h"
#include "wifi.h"
#include "mqtt.h"
#include "ota.h"
#include "debug.h"

MQTT_Client mqttClient;

void ICACHE_FLASH_ATTR wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}

void ICACHE_FLASH_ATTR mqttPublishSettings() {
	char buff[100] = "";

	os_sprintf(buff, "{\"relay1\":%d,\"relay2\":%d,\"relay3\":%d,\"relay4\":%d,\"rom\":%d,\"device\":\"%s\"}",
		sysCfg.relay1, 
		sysCfg.relay2, 
		sysCfg.relay3, 
		sysCfg.relay4, 
		rboot_get_current_rom(),
		DEVICE
	);

	MQTT_Publish(&mqttClient, MQTT_TOPIC_SETTINGS_REPLY, buff, os_strlen(buff), 0, 0);
	INFO("MQTT send topic: %s, data: %s \r\n", MQTT_TOPIC_SETTINGS_REPLY, buff);
}

void ICACHE_FLASH_ATTR mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected\r\n");

	MQTT_Subscribe(client, MQTT_TOPIC_RELAY_1, 0);
	MQTT_Subscribe(client, MQTT_TOPIC_RELAY_2, 0);
	MQTT_Subscribe(client, MQTT_TOPIC_RELAY_3, 0);
	MQTT_Subscribe(client, MQTT_TOPIC_RELAY_4, 0);

	MQTT_Subscribe(client, MQTT_TOPIC_SETTINGS, 0);

	MQTT_Subscribe(client, MQTT_TOPIC_RESTART, 0);

	MQTT_Subscribe(client, MQTT_TOPIC_UPDATE, 0);

	mqttPublishSettings(args);
}

void ICACHE_FLASH_ATTR mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Disconnected\r\n");
}

void ICACHE_FLASH_ATTR mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");
}



void ICACHE_FLASH_ATTR mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len + 1),
	     *dataBuf  = (char*)os_zalloc(data_len + 1);

	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	INFO("Receive topic: %s, data: %s\r\n", topicBuf, dataBuf);

	if (os_strcmp(topicBuf, MQTT_TOPIC_RELAY_1) == 0)
	{
		sysCfg.relay1 = (atoi(dataBuf) == 0 ? 0 : 1);

		GPIO_OUTPUT_SET(RELAY_1_PIN, sysCfg.relay1);

		CFG_Save();
		mqttPublishSettings();
	} else
	if (os_strcmp(topicBuf, MQTT_TOPIC_RELAY_2) == 0)
	{
		sysCfg.relay2 = (atoi(dataBuf) == 0 ? 0 : 1);

		GPIO_OUTPUT_SET(RELAY_2_PIN, sysCfg.relay2);

		CFG_Save();
		mqttPublishSettings();
	} else
	if (os_strcmp(topicBuf, MQTT_TOPIC_RELAY_3) == 0)
	{
		sysCfg.relay3 = (atoi(dataBuf) == 0 ? 0 : 1);

		GPIO_OUTPUT_SET(RELAY_3_PIN, sysCfg.relay3);

		CFG_Save();
		mqttPublishSettings();
	} else
	if (os_strcmp(topicBuf, MQTT_TOPIC_RELAY_4) == 0)
	{
		sysCfg.relay4 = (atoi(dataBuf) == 0 ? 0 : 1);

		GPIO_OUTPUT_SET(RELAY_4_PIN, sysCfg.relay4);

		CFG_Save();
		mqttPublishSettings();
	} else
	if (os_strcmp(topicBuf, MQTT_TOPIC_SETTINGS) == 0)
	{
		mqttPublishSettings();
	} else
	if (os_strcmp(topicBuf, MQTT_TOPIC_UPDATE) == 0)
	{
		OtaUpdate();
	} else
	if (os_strcmp(topicBuf, MQTT_TOPIC_RESTART) == 0)
	{
		system_restart();
	}

	os_free(topicBuf);
	os_free(dataBuf);
}

void ICACHE_FLASH_ATTR
user_init()
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000000);

	INFO("Starting up...\r\n");

	INFO("Loading config...\r\n");
	CFG_Load();

	gpio_init();

	INFO("Initializing MQTT...\r\n");
	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

	INFO("Connect to WIFI...\r\n");
	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

	// init last state
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);

	GPIO_OUTPUT_SET(RELAY_1_PIN, sysCfg.relay1);
	GPIO_OUTPUT_SET(RELAY_2_PIN, sysCfg.relay2);
	GPIO_OUTPUT_SET(RELAY_3_PIN, sysCfg.relay3);
	GPIO_OUTPUT_SET(RELAY_4_PIN, sysCfg.relay4);

	INFO("Startup completed. Now running from rom %d...\r\n", rboot_get_current_rom());
}
