//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 6/15/20.
//
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <cJSON.h>

// This defines enables prototype integration with iotc-c-lib v3.0.0
//#define PROTOCOL_V2_PROTOTYPE

#include "iotconnect_discovery.h"
#include "iotconnect_event.h"
#include "iotc_http_client.h"
#include "iotc_mqtt_client.h"
#include "iotconnect.h"

#define IOTC_DISCOVERY_PATH_FORMAT "/api/sdk/cpid/%s/lang/M_C/ver/2.0/env/%s"
#define IOTC_SYNC_PATH_FORMAT "%ssync?"

static IotclDiscoveryResponse *discovery_response = NULL;
static IotclSyncResponse *sync_response = NULL;
static IotConnectClientConfig config = { 0 };
static IotclConfig lib_config = { 0 };

static void dump_response(const char *message, IotConnectHttpResponse *response) {
    printf("%s", message);
    if (response->data) {
        printf(" Response was:\n----\n%s\n----\n", response->data);
    } else {
        printf(" Response was empty\n");
    }
}

static void report_sync_error(IotclSyncResponse *response, const char *sync_response_str) {
    if (NULL == response) {
        printf("Failed to obtain sync response?\n");
        return;
    }
    switch (response->ds) {
    case IOTCL_SR_DEVICE_NOT_REGISTERED:
        printf("IOTC_SyncResponse error: Not registered\n");
        break;
    case IOTCL_SR_AUTO_REGISTER:
        printf("IOTC_SyncResponse error: Auto Register\n");
        break;
    case IOTCL_SR_DEVICE_NOT_FOUND:
        printf("IOTC_SyncResponse error: Device not found\n");
        break;
    case IOTCL_SR_DEVICE_INACTIVE:
        printf("IOTC_SyncResponse error: Device inactive\n");
        break;
    case IOTCL_SR_DEVICE_MOVED:
        printf("IOTC_SyncResponse error: Device moved\n");
        break;
    case IOTCL_SR_CPID_NOT_FOUND:
        printf("IOTC_SyncResponse error: CPID not found\n");
        break;
    case IOTCL_SR_UNKNOWN_DEVICE_STATUS:
        printf("IOTC_SyncResponse error: Unknown device status error from server\n");
        break;
    case IOTCL_SR_ALLOCATION_ERROR:
        printf("IOTC_SyncResponse internal error: Allocation Error\n");
        break;
    case IOTCL_SR_PARSING_ERROR:
        printf("IOTC_SyncResponse internal error: Parsing error. Please check parameters passed to the request.\n");
        break;
    default:
        printf("WARN: report_sync_error called, but no error returned?\n");
        break;
    }
    printf("Raw server response was:\n--------------\n%s\n--------------\n", sync_response_str);
}

static IotclDiscoveryResponse* run_http_discovery(const char *cpid, const char *env) {
    IotclDiscoveryResponse *ret = NULL;
    char *url_buff = malloc(sizeof(IOTC_DISCOVERY_PATH_FORMAT) + strlen(cpid) + strlen(env) - 4 /* %s x 2 */);

    sprintf(url_buff, IOTC_DISCOVERY_PATH_FORMAT, cpid, env);

    IotConnectHttpResponse response;
    iotconnect_https_request(&response, IOTCONNECT_DISCOVERY_HOSTNAME, url_buff, NULL);

    if (NULL == response.data) {
        dump_response("Unable to parse HTTP response,", &response);
        goto cleanup;
    }
    char *json_start = strstr(response.data, "{");
    if (NULL == json_start) {
        dump_response("No json response from server.", &response);
        goto cleanup;
    }
    if (json_start != response.data) {
        dump_response("WARN: Expected JSON to start immediately in the returned data.", &response);
    }

    ret = iotcl_discovery_parse_discovery_response(json_start);

    cleanup: iotconnect_free_https_response(&response);
    // fall through
    return ret;
}

static IotclSyncResponse* run_http_sync(const char *cpid, const char *uniqueid) {
    IotclSyncResponse *ret = NULL;

    char *path_buff = malloc(sizeof(IOTC_SYNC_PATH_FORMAT) + strlen(discovery_response->path));
    char *post_data = malloc(IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN + 1);

    if (!path_buff || !post_data) {
        fprintf(stderr, "run_http_sync: Out of memory!");
        free(path_buff); // one of them could have succeeded
        free(post_data);
        return NULL;
    }

    sprintf(path_buff, IOTC_SYNC_PATH_FORMAT, discovery_response->path);

    snprintf(post_data, //
            IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN, //
            IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_TEMPLATE, //
            cpid, uniqueid //
            );

    IotConnectHttpResponse response;
    iotconnect_https_request(&response, discovery_response->host, path_buff, post_data);

    free(path_buff);
    free(post_data);

    if (NULL == response.data) {
        dump_response("Unable to parse HTTP response.", &response);
        goto cleanup;
    }
    char *json_start = strstr(response.data, "{");
    if (NULL == json_start) {
        dump_response("No json response from server.", &response);
        goto cleanup;
    }
    if (json_start != response.data) {
        dump_response("WARN: Expected JSON to start immediately in the returned data.", &response);
    }

    ret = iotcl_discovery_parse_sync_response(json_start);
    if (!ret || ret->ds != IOTCL_SR_OK) {
        report_sync_error(ret, response.data);
        iotcl_discovery_free_sync_response(ret);
        ret = NULL;
    }

    cleanup: iotconnect_free_https_response(&response);
    // fall through

    return ret;
}

static void on_mqtt_data(const char *message, size_t message_len) {
    char *str = malloc(message_len + 1);
    memcpy(str, message, message_len);
    str[message_len] = 0;
    printf("event>>> %s\n", str);
    if (!iotcl_process_event(str)) {
        printf("Error encountered while processing %s\n", str);
    }
    free(str);
}

static void on_iotconnect_status(IotConnectConnectionStatus status) {
    if (config.status_cb) {
        config.status_cb(status);
    }
}

cy_rslt_t iotconnect_sdk_disconnect() {
    return iotc_mqtt_client_disconnect();
}

cy_rslt_t iotconnect_sdk_send_packet(const char *data) {
    return iotc_mqtt_client_publish(data, 1);
}

cy_rslt_t iotconnect_sdk_send_packet_qos(const char *data, int qos) {
    return iotc_mqtt_client_publish(data, qos);
}

static void on_message_intercept(IotclEventData data, IotConnectEventType type) {
    switch (type) {
    case ON_FORCE_SYNC:
        iotconnect_sdk_disconnect();
        iotcl_discovery_free_sync_response(sync_response);
        sync_response = NULL;
        sync_response = run_http_sync(config.cpid, config.duid);
        if (NULL == sync_response) {
            printf("Unable to run HTTP sync on ON_FORCE_SYNC \n");
            return;
        }
        printf("Got ON_FORCE_SYNC. Disconnecting.\n");
        iotconnect_sdk_disconnect(); // client will get notification that we disconnected and will reinit
        break;
    case ON_CLOSE:
        printf("Got a disconnect request. Closing the mqtt connection. Device restart is required.\n");
        iotconnect_sdk_disconnect();
        break;
    default:
        break; // not handling nay other messages
    }

    if (NULL != config.msg_cb) {
        config.msg_cb(data, type);
    }
}

IotclConfig* iotconnect_sdk_get_lib_config() {
    return iotcl_get_config();
}

IotConnectClientConfig* iotconnect_sdk_init_and_get_config() {
    memset(&config, 0, sizeof(config));
    return &config;
}

bool iotconnect_sdk_is_connected() {
    return iotc_mqtt_client_is_connected();
}

bool validate_auth_type() {
    bool ret;
    /* todo fix iotc lib for "at" response */
#if 0
    IotConnectAuthType mapped = (IotConnectAuthType) sync_response->at;
    if (sync_response->at == 3 /* ca cert or self signed is the same from this perspective */) {
        mapped = IOTC_AT_X509;
    }
#endif

    switch (config.auth.type) {
    case IOTC_AT_TOKEN:
        printf("WARNING: Token authentication should not be used in production\n");
        ret = true; // mapped == config.auth.type;
        break;
    case IOTC_AT_X509:
        ret = true; // mapped == config.auth.type;
        break;
    case IOTC_AT_SYMMETRIC_KEY:
        printf("ERROR: Token authentication should not be used in production\n");
        ret = false;
        break;
    default:
        printf("ERROR: Unknown authentication type %d\n", sync_response->at);
        ret = false;
        break;
    }
    return ret;
}

///////////////////////////////////////////////////////////////////////////////////
// this the Initialization os IoTConnect SDK
int iotconnect_sdk_init() {

    if (!discovery_response) {
        discovery_response = run_http_discovery(config.cpid, config.env);
        if (NULL == discovery_response) {
            // get_base_url will print the error
            return -1;
        }
        printf("Discovery response parsing successful.\n");
    }
    if (!sync_response) {
        sync_response = run_http_sync(config.cpid, config.duid);
        if (NULL == sync_response) {
            // Sync_call will print the error
            return -2;
        }
        printf("Sync response parsing successful.\n");
    }

    if (!validate_auth_type()) {
        return -3;
    }
    char cpid_buff[5];
    strncpy(cpid_buff, sync_response->cpid, 4);
    cpid_buff[4] = 0;
    printf("CPID: %s***\n", cpid_buff);
    printf("ENV:  %s\n", config.env);

    if (!config.env || !config.cpid || !config.duid) {
        printf("Error: Device configuration is invalid. Configuration values for env, cpid and duid are required.\n");
        return -4;
    }

    lib_config.device.env = config.env;
    lib_config.device.cpid = config.cpid;
    lib_config.device.duid = config.duid;

    lib_config.event_functions.ota_cb = config.ota_cb;
    lib_config.event_functions.cmd_cb = config.cmd_cb;
    lib_config.event_functions.msg_cb = on_message_intercept;

    lib_config.telemetry.dtg = sync_response->dtg;

    int ret = iotcl_init(&lib_config);
    if (!ret) {
        printf("Failed to initialize the IoTConnect Lib\n");
        return -5;
    }

    IotConnectMqttConfig mqtt_config = { 0 };
    mqtt_config.auth = &config.auth;
    mqtt_config.sr = sync_response;
    mqtt_config.c2d_msg_cb = on_mqtt_data;
    mqtt_config.status_cb = on_iotconnect_status;
    cy_rslt_t ret_cy = iotc_mqtt_client_init(&mqtt_config);
    if (ret_cy) {
        printf("Failed to connect!\n");
        ret = -6;
    }
    return 0;

}
