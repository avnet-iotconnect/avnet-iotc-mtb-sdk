/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTC_MQTT_CLIENT_H
#define IOTC_MQTT_CLIENT_H

#include <stddef.h>
#include "cy_result.h"
#include "iotconnect.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*IotConnectMqttInboundMessageCallback)(const char* topic, const char *message, size_t message_len);

typedef struct {
	IotConnectConnectionType connection_type; // AWS or Azure
	IotConnectX509Config *x509_config; // Pointer to IoTConnect c509 configuration
    IotConnectMqttInboundMessageCallback mqtt_inbound_msg_cb; // callback for inbound MQTT messages
    IotConnectStatusCallback status_cb; // callback for connection status
} IotConnectMqttConfig;

cy_rslt_t iotc_mqtt_client_init(IotConnectMqttConfig* mqtt_config);

cy_rslt_t iotc_mqtt_client_disconnect();

bool iotc_mqtt_client_is_connected();

// send a null terminated string
cy_rslt_t iotc_mqtt_client_publish(const char * topic, const char *payload, int qos);

#ifdef __cplusplus
}
#endif

#endif // IOTC_MQTT_CLIENT_H
