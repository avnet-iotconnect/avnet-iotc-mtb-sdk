//
// Copyright: Avnet 2021
// Created by Nik Markovic <nikola.markovic@avnet.com> on 11/11/21.
//
#ifndef IOTC_MQTT_CLIENT_H
#define IOTC_MQTT_CLIENT_H

#include <stddef.h>
#include "cy_result.h"
#include "iotconnect.h"
#include "iotconnect_discovery.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*IotConnectC2dCallback)(const char* message, size_t message_len);

typedef struct {
    IotclSyncResponse* sr;
    IotConnectAuthInfo *auth; // Pointer to IoTConnect auth configuration
    IotConnectC2dCallback c2d_msg_cb; // callback for inbound messages
    IotConnectStatusCallback status_cb; // callback for connection status
} IotConnectMqttConfig;

cy_rslt_t iotc_mqtt_client_init(IotConnectMqttConfig* mqtt_config);

cy_rslt_t iotc_mqtt_client_disconnect();

bool iotc_mqtt_client_is_connected();

// send a null terminated string
cy_rslt_t iotc_mqtt_client_publish(const char *payload, int qos);

#ifdef __cplusplus
}
#endif

#endif // IOTC_MQTT_CLIENT_H