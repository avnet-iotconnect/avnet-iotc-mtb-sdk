/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTCONNECT_H
#define IOTCONNECT_H

#include "cy_result.h"
#include "cyabs_rtos.h" // for cy_time_t
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
	IOTC_CT_UNDEFINED = 0,
    IOTC_CT_AWS,
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
    IotConnectX509Config x509_config; // NOTE: The user must maintain references to all certificates until sdk is deinitialized.
    IoTConnectCallbacks callbacks;

    // QOS for outbound messages. Default 1.
    int qos;

    // up to how many inbound messages (default 4) to queue up into the message queue for offloaded processing:
    size_t mq_max_messages;

    bool verbose; // If true, we will output extra info and sent and received MQTT json data to standard out
} IotConnectClientConfig;


void iotconnect_sdk_init_config(IotConnectClientConfig * c);

// call iotconnect_sdk_init_config first and configure the SDK before calling iotconnect_sdk_init()
// NOTE: the client needs to keep references to all certificates, but does not need to keep references to other configuration pointers.
int iotconnect_sdk_init(IotConnectClientConfig * c);

cy_rslt_t iotconnect_sdk_connect(void);

// The client code should periodically poll the message queue for inbound messages (commands OTA etc.)
// This all will serve all messages in the queue (if any) and call appropriate command/OTA callbacks
// Wait up to timeout_ms milliseconds, and if messages are available, processes them with itc-c-lib
// If timeout_ms is zero, the call will block forever until a message arrives
void iotconnect_sdk_poll_inbound_mq(cy_time_t timeout_ms);

bool iotconnect_sdk_is_connected(void);

cy_rslt_t iotconnect_sdk_disconnect(void);

void iotconnect_sdk_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
