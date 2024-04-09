/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <cJSON.h>

// This defines enables prototype integration with iotc-c-lib v3.0.0
//#define PROTOCOL_V2_PROTOTYPE

#include "iotcl.h"
#include "iotcl_dra_discovery.h"
#include "iotcl_dra_identity.h"
#include "iotc_http_client.h"
#include "iotc_mqtt_client.h"
#include "iotconnect.h"

// local variables determined by the last client config
static bool verbose = false;
static int send_qos = 0;

static void default_on_connection_status(IotConnectConnectionStatus status) {
    // Add your own status handling
    switch (status) {
        case IOTC_CS_MQTT_CONNECTED:
            printf("IoTConnect Client Connected notification.\n");
            break;
        case IOTC_CS_MQTT_DISCONNECTED:
            printf("IoTConnect Client Disconnected notification.\n");
            break;
        default:
            printf("IoTConnect Client ERROR notification\n");
            break;
    }
}


static void dump_response(const char *message, IotConnectHttpResponse *response) {
    if (message) {
        printf("%s\n", message);
    }

    if (response->data) {
        printf(" Response was:\n----\n%s\n----\n", response->data);
    } else {
    	printf(" Response was empty\n");
    }
}


static int validate_response(IotConnectHttpResponse *response) {
    if (NULL == response->data) {
        dump_response("Unable to parse HTTP response.", response);
        return IOTCL_ERR_PARSING_ERROR;
    }
    const char *json_start = strstr(response->data, "{");
    if (NULL == json_start) {
        dump_response("No json response from server.", response);
        return IOTCL_ERR_PARSING_ERROR;
    }
    if (json_start != response->data) {
        dump_response("WARN: Expected JSON to start immediately in the returned data.", response);
    }
    return IOTCL_SUCCESS;
}

static int run_http_identity(IotConnectConnectionType ct, const char *cpid, const char *env, const char* duid) {
    IotConnectHttpResponse response = {0};
    IotclDraUrlContext discovery_url = {0};
    IotclDraUrlContext identity_url = {0};
    int status;
    switch (ct) {
        case IOTC_CT_AWS:
        printf("Using AWS discovery URL...\n");
            status = iotcl_dra_discovery_init_url_aws(&discovery_url, cpid, env);
            break;
        case IOTC_CT_AZURE:
        printf("Using Azure discovery URL...\n");
            status = iotcl_dra_discovery_init_url_azure(&discovery_url, cpid, env);
            break;
        default:
        printf("Unknown connection type %d\n", ct);
            return IOTCL_ERR_BAD_VALUE;
    }

    if (status) {
        return status; // called function will print the error
    }

    iotconnect_https_request(&response,
                             iotcl_dra_url_get_hostname(&discovery_url),
							 iotcl_dra_url_get_resource(&discovery_url),
							 NULL
    );

    status = validate_response(&response);
    if (status) goto cleanup; // called function will print the error


    status = iotcl_dra_discovery_parse(&identity_url, 0, response.data);
    if (status) {
        printf("Error while parsing discovery response from %s\n", iotcl_dra_url_get_url(&discovery_url));
        dump_response(NULL, &response);
        goto cleanup;
    }

    iotconnect_free_https_response(&response);
    memset(&response, 0, sizeof(response));

    status = iotcl_dra_identity_build_url(&identity_url, duid);
    if (status) goto cleanup; // called function will print the error

    iotconnect_https_request(&response,
                             iotcl_dra_url_get_hostname(&discovery_url),
							 iotcl_dra_url_get_resource(&discovery_url),
							 NULL
    );

    status = validate_response(&response);
    if (status) goto cleanup; // called function will print the error

    status = iotcl_dra_identity_configure_library_mqtt(response.data);
    if (status) {
        printf("Error while parsing identity response from %s\n", iotcl_dra_url_get_url(&identity_url));
        dump_response(NULL, &response);
        goto cleanup;
    }

    if (ct == IOTC_CT_AWS && iotcl_mqtt_get_config()->username) {
        // workaround for identity returning username for AWS.
        // https://awspoc.iotconnect.io/support-info/2024036163515369
        iotcl_free(iotcl_mqtt_get_config()->username);
        iotcl_mqtt_get_config()->username = NULL;
    }

    cleanup:
    iotcl_dra_url_deinit(&discovery_url);
    iotcl_dra_url_deinit(&identity_url);
    iotconnect_free_https_response(&response);
    return status;
}

static void on_mqtt_c2d_message(const char* topic, const char *message, size_t message_len) {
    if (verbose) {
        printf("<: %.*s\n", (int) message_len, message);
    }
    iotcl_c2d_process_event_with_length((uint8_t*) message, message_len);
}

void iotconnect_sdk_mqtt_send_cb(const char *topic, const char *json_str) {
    if (verbose) {
        printf(">: %s\n",  json_str);
    }
    iotc_mqtt_client_publish(topic, json_str, send_qos);
}

cy_rslt_t iotconnect_sdk_disconnect() {
    return (cy_rslt_t) iotc_mqtt_client_disconnect();
}


void iotconnect_sdk_init_config(IotConnectClientConfig *c) {
    memset(c, 0, sizeof(IotConnectClientConfig));
    c->qos = 1;
}

bool iotconnect_sdk_is_connected() {
    return iotc_mqtt_client_is_connected();
}

int iotconnect_sdk_init(IotConnectClientConfig *c) {
	int status;
    if (!c->env || !c->cpid || !c->duid) {
        printf("Error: Device configuration is invalid. Configuration values for env, cpid and duid are required.");
        iotconnect_sdk_deinit();
        return IOTCL_ERR_MISSING_VALUE;
    }

    if (c->connection_type != IOTC_CT_AWS && c->connection_type != IOTC_CT_AZURE) {
        printf("Error: Device configuration is invalid. Must set connection type");
        iotconnect_sdk_deinit();
        return IOTCL_ERR_MISSING_VALUE;
    }

    IotclClientConfig iotcl_cfg;
	iotcl_init_client_config(&iotcl_cfg);
	iotcl_cfg.device.cpid = c->cpid;
	iotcl_cfg.device.duid = c->duid;
	iotcl_cfg.device.instance_type = IOTCL_DCT_CUSTOM;
	iotcl_cfg.mqtt_send_cb = iotconnect_sdk_mqtt_send_cb;
	iotcl_cfg.events.cmd_cb = c->callbacks.cmd_cb;
	iotcl_cfg.events.ota_cb = c->callbacks.ota_cb;

    send_qos = c->qos;
    verbose = c->verbose;

    if (c->verbose) {
        status = iotcl_init_and_print_config(&iotcl_cfg);
    } else {
        status = iotcl_init(&iotcl_cfg);
    }

	status = run_http_identity(c->connection_type, c->duid, c->cpid, c->env);
    if (status) {
		iotconnect_sdk_deinit();
        return status;
    }
    printf("Identity response parsing successful.\n");

    IotConnectMqttConfig mqtt_config = { 0 };
    mqtt_config.x509_config = &c->x509_config;
    mqtt_config.connection_type = c->connection_type;
    mqtt_config.mqtt_inbound_msg_cb = on_mqtt_c2d_message;
    mqtt_config.status_cb = c->callbacks.status_cb ? c->callbacks.status_cb : default_on_connection_status;
    cy_rslt_t ret_cy = iotc_mqtt_client_init(&mqtt_config);
    if (ret_cy) {
		iotconnect_sdk_deinit();
		printf("Failed to connect!\n");
        return IOTCL_ERR_FAILED;
    }
    return 0;
}

void iotconnect_sdk_deinit(void) {
	if (iotconnect_sdk_is_connected()) {
		iotconnect_sdk_disconnect();
	}
	iotcl_deinit();
}
