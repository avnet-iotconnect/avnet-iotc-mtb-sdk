/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTCONNECT_H
#define IOTCONNECT_H

#include "cy_result.h"
#include "cyabs_rtos.h" // for cy_time_t
#include "iotcl.h"
#include "iotcl_dra_credentials.h"

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
    const char* device_cert; // CA cert (or chain) in PEM format
    const char* device_key; // Device private key either in PEM format or as an  MbedTLS opaque key (see device_key_size).
    size_t device_key_size; // If using a PEM private key, you should leave this value at zero. If using opaque keys or similar, set this accordingly.
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

// Obtain temporary AWS credentials for Video Streaming via mTLS GET to /IOTCONNECT.
// Requires connection_type IOTC_CT_AWS and that iotcl_mqtt_get_config()->aws.vs_creds_url is non-NULL
// (set by the identity response when "Video Streaming" + "WebRTC" template options are enabled).
// Uses x509_config.device_cert/device_key for mTLS. Server CA defaults to AmazonRootCA1, but if
// x509_config.server_ca_cert is non-NULL it is used instead (lets the user override with a renewed cert).
// On success the result is cached and freed on iotconnect_sdk_deinit() or iotconnect_sdk_aws_creds_free().
// Calling again replaces the cached value without leaking. Returns IOTCL_SUCCESS or an IOTCL_ERR_* code.
int iotconnect_sdk_obtain_aws_creds(void);

// Returns pointer to the most recently obtained creds, or NULL if no creds are cached or if
// the cached creds have expired. If expired, the cache is freed before returning NULL and an
// error is printed. Pointer is owned by the SDK. Do not free.
const IotclDraCredentialsResult *iotconnect_sdk_aws_creds_get(void);

// Returns: positive seconds remaining until expiration if creds are still valid;
// 0 if creds have expired (regardless of whether they have been freed);
// -1 if creds were never obtained.
// Behavior is identical regardless of whether iotconnect_sdk_aws_creds_free() was called.
int iotconnect_sdk_aws_creds_seconds_until_expiry(void);

// Frees the cached creds RAM. The cached expiration time is retained, so
// iotconnect_sdk_aws_creds_seconds_until_expiry() continues to report based on the last expiry.
void iotconnect_sdk_aws_creds_free(void);

#ifdef __cplusplus
}
#endif

#endif
