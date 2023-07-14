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


#define PUB_TOPIC_FORMAT	"devices/%s/messages/events/"
#define SUB_TOPIC_FORMAT	"iot/%s/cmd"
#define USER_NAME_FORMAT    "%s/%s"

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
static IotclSyncResponse* populate_sync_response_aws(void) {
    IotclSyncResponse* aws_response = (IotclSyncResponse* ) calloc(1, sizeof(IotclSyncResponse));
    if (NULL == aws_response) {
        return NULL;
    }

    aws_response->broker.host = (char* )malloc(strlen(config.host) + 1);
	if (NULL == aws_response->broker.host) {
		goto cleanup;
	}
	strcpy(aws_response->broker.host, config.host);

	aws_response->broker.client_id = (char* )malloc(strlen(config.duid) + 1);
	if (NULL == aws_response->broker.client_id) {
		goto cleanup;
	}
	strcpy(aws_response->broker.client_id, config.duid);

	aws_response->broker.user_name = (char* )malloc(sizeof(USER_NAME_FORMAT) + strlen(config.duid) + strlen(config.host) - 4);
	if (NULL == aws_response->broker.user_name) {
		goto cleanup;
	}
    sprintf(aws_response->broker.user_name, USER_NAME_FORMAT, config.host, config.duid);

    aws_response->broker.sub_topic = (char* )malloc(sizeof(SUB_TOPIC_FORMAT) + strlen(config.duid) - 2);
	if (NULL == aws_response->broker.sub_topic) {
		goto cleanup;
	}
	sprintf(aws_response->broker.sub_topic, SUB_TOPIC_FORMAT, config.duid);

	aws_response->broker.pub_topic = (char* )malloc(sizeof(PUB_TOPIC_FORMAT) + strlen(config.duid) - 2);
	if (NULL == aws_response->broker.pub_topic) {
		goto cleanup;
	}
	sprintf(aws_response->broker.pub_topic, PUB_TOPIC_FORMAT, config.duid);

	aws_response->broker.pass = NULL;

	return aws_response;

	cleanup:
	iotcl_discovery_free_sync_response(aws_response);
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////
// this the Initialization os IoTConnect SDK
int iotconnect_sdk_init(void) {
/*
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
*/
    if (!validate_auth_type()) {
        return -3;
    }

#if 0
    char cpid_buff[5];
    strncpy(cpid_buff, sync_response->cpid, 4);
    cpid_buff[4] = 0;
    printf("CPID: %s***\n", cpid_buff);
    printf("ENV:  %s\n", config.env);

    if (!config.env || !config.cpid || !config.duid) {
        printf("Error: Device configuration is invalid. Configuration values for env, cpid and duid are required.\n");
        return -4;
    }
#endif

    lib_config.device.duid = config.duid;

    lib_config.event_functions.ota_cb = config.ota_cb;
    lib_config.event_functions.cmd_cb = config.cmd_cb;
    lib_config.event_functions.msg_cb = on_message_intercept;

    lib_config.telemetry.cd = config.cd;

    int ret = iotcl_init_v2(&lib_config);
    if (!ret) {
        printf("Failed to initialize the IoTConnect Lib\n");
        return -5;
    }

    if (!sync_response) {
        sync_response = populate_sync_response_aws();
        if (NULL == sync_response) {
            return -2;
        }
        printf("Sync response parsing successful.\n");
    }

	printf("\nHost is %s\n", sync_response->broker.host);
	printf("client_id is %s\n", sync_response->broker.client_id);
	printf("user name is %s\n", sync_response->broker.user_name);
	printf("pub_topic is %s\n", sync_response->broker.pub_topic);
	printf("sub_topic is %s\n", sync_response->broker.sub_topic);

    IotConnectMqttConfig mqtt_config = { 0 };
    mqtt_config.auth = &config.auth;
    mqtt_config.sr = sync_response;
    mqtt_config.c2d_msg_cb = on_mqtt_data;
    mqtt_config.status_cb = on_iotconnect_status;
    cy_rslt_t ret_cy = iotc_mqtt_client_init(&mqtt_config);
    if (ret_cy) {
        printf("Failed to connect!\n");
        return -6;
    }
    return 0;

}
