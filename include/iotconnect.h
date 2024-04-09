/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTCONNECT_H
#define IOTCONNECT_H

#include "cy_result.h"
#include "iotcl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IOTC_CS_UNDEFINED,
    IOTC_CS_MQTT_CONNECTED,
    IOTC_CS_MQTT_DISCONNECTED
} IotConnectConnectionStatus;

typedef enum {
    IOTC_CT_AWS = 1,
    IOTC_CT_AZURE
} IotConnectConnectionType;

typedef void (*IotConnectStatusCallback)(IotConnectConnectionStatus data);

typedef struct {
	const char* server_ca_cert; // OPTIONAL server cert that will default to AmazonRootCA1 or Digicert G2 depending on connection type
	const char* device_cert; // Path to a file containing the device CA cert (or chain) in PEM format
	const char* device_key; // Path to a file containing the device private key in PEM format
} IotConnectX509Config;


typedef struct {
    IotclOtaCallback ota_cb; // callback for OTA events.
    IotclCommandCallback cmd_cb; // callback for command events.
    IotConnectStatusCallback status_cb; // callback for connection status
} IoTConnectCallbacks;

typedef struct {
    const char *env;    // Environment name from Settings->Key Vault.
    const char *cpid;   // CPID from Settings->Key Vault.
    const char *duid;   // Name of the device.
    IotConnectConnectionType connection_type;
    IotConnectX509Config x509_config;
    IoTConnectCallbacks callbacks;
    int qos; // QOS for outbound messages. Default 1.
    bool verbose; // If true, we will output extra info and sent and received MQTT json data to standard out
} IotConnectClientConfig;


void iotconnect_sdk_init_config(IotConnectClientConfig * c);

// call iotconnect_sdk_init_config first and configure the SDK before calling iotconnect_sdk_init()
// NOTE: the client does not need to keep references to the struct or any values inside it
int iotconnect_sdk_init(IotConnectClientConfig * c);

cy_rslt_t iotconnect_sdk_connect(void);

bool iotconnect_sdk_is_connected(void);

cy_rslt_t iotconnect_sdk_disconnect(void);

void iotconnect_sdk_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
