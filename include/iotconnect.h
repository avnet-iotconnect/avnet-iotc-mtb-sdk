//
// Copyright: Avnet 2021
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 11/11/21.
//

#ifndef IOTCONNECT_H
#define IOTCONNECT_H

#include <stddef.h>
#include "iotconnect_event.h"
#include "iotconnect_telemetry.h"
#include "iotconnect_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IOTC_CS_UNDEFINED,
    IOTC_CS_MQTT_CONNECTED,
    IOTC_CS_MQTT_DISCONNECTED
} IotConnectConnectionStatus;


typedef enum {
    // Authentication based on your CPID. Sync HTTP endpoint returns a long lived SAS token
    // This auth type is only intended as a simple way to connect your test and development devices
    // and must not be used in production
    IOTC_AT_TOKEN = 1,

    // CA Cert and Self Signed Cert
    IOTC_AT_X509 = 2,

    // NOT SUPPORTED. Reserved.
    // TPM hardware devices -
    IOTC_AT_TPM = 4, // 4 for compatibility with sync

    // NOT SUPPORTED. Reserved.
    // IoTHub Key based authentication with Symmetric Keys (Primary or Secondary key)
    IOTC_AT_SYMMETRIC_KEY = 5

} IotConnectAuthType;




typedef struct {
    IotConnectAuthType type;
    union {
        struct {
            const char* device_cert; // Path to a file containing the device CA cert (or chain) in PEM format
            const char* device_key; // Path to a file containing the device private key in PEM format
        } cert_info;
        char *symmetric_key;
    } data;
} IotConnectAuthInfo;

typedef void (*IotConnectStatusCallback)(IotConnectConnectionStatus data);

typedef struct {
    char *env;    // Environment name. Contact your representative for details.
    char *host;    // Environment name. Contact your representative for details.
    char *cpid;   // Settings -> Company Profile.
    char *duid;   // Name of the device.
    IotConnectAuthInfo auth;
    IotclOtaCallback ota_cb; // callback for OTA events.
    IotclCommandCallback cmd_cb; // callback for command events.
    IotclMessageCallback msg_cb; // callback for ALL messages, including the specific ones like cmd or ota callback.
    IotConnectStatusCallback status_cb; // callback for connection status
} IotConnectClientConfig;


IotConnectClientConfig *iotconnect_sdk_init_and_get_config();

int iotconnect_sdk_init();

bool iotconnect_sdk_is_connected();

IotclConfig *iotconnect_sdk_get_lib_config();

cy_rslt_t iotconnect_sdk_send_packet(const char *data);
cy_rslt_t iotconnect_sdk_send_packet_int(const char *data, int qos);

cy_rslt_t iotconnect_sdk_disconnect();

#ifdef __cplusplus
}
#endif

#endif
