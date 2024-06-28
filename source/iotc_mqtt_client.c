/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include "cyhal.h"
#include "cybsp.h"

/* FreeRTOS header files */
#include "FreeRTOS.h"
#include "task.h"

/* Middleware libraries */
#include "cy_retarget_io.h"

#include "cy_mqtt_api.h"
#include "clock.h"

/* LwIP header files */
#include "lwip/netif.h"

#include "iotcl_certs.h"
#include "iotc_mqtt_client.h"

/* Maximum number of retries for MQTT subscribe operation */
#define MAX_SUBSCRIBE_RETRIES                   (3u)

/* Time interval in milliseconds between MQTT subscribe retries. */
#define MQTT_SUBSCRIBE_RETRY_INTERVAL_MS        (1000)

/* Queue length of a message queue that is used to communicate with the
 * subscriber task.
 */
#define SUBSCRIBER_TASK_QUEUE_LENGTH            (1u)

#define MQTT_NETWORK_BUFFER_SIZE          ( 4 * CY_MQTT_MIN_NETWORK_BUFFER_SIZE )

/* Maximum MQTT connection re-connection limit. */
#ifndef IOTC_MAX_MQTT_CONN_RETRIES
#define IOTC_MAX_MQTT_CONN_RETRIES            (150u)
#endif

/* MQTT re-connection time interval in milliseconds. */
#ifndef IOTC_MQTT_CONN_RETRY_INTERVAL_MS
#define IOTC_MQTT_CONN_RETRY_INTERVAL_MS      (5000)
#endif

/*String that describes the MQTT handle that is being created in order to uniquely identify it*/
#define MQTT_HANDLE_DESCRIPTOR            "IoTConnect"

static cy_mqtt_t mqtt_connection;
static uint8_t mqtt_network_buffer[MQTT_NETWORK_BUFFER_SIZE];
static bool is_connected = false;
static bool is_disconnect_requested = false;
static bool is_mqtt_initialized = false;
static IotConnectMqttInboundMessageCallback mqtt_inbound_msg_cb = NULL; // callback for inbound messages
static IotConnectStatusCallback status_cb = NULL; // callback for connection status

static void mqtt_event_callback(cy_mqtt_t mqtt_handle, cy_mqtt_event_t event, void *user_data) {
    (void) mqtt_handle;
    (void) user_data;
    switch (event.type) {
		case CY_MQTT_EVENT_TYPE_DISCONNECT: {
			/* MQTT connection with the MQTT broker is broken as the client
			 * is unable to communicate with the broker. Set the appropriate
			 * command to be sent to the MQTT task.
			 */
			printf("Unexpectedly disconnected from MQTT broker!\n");

			/* Send the message to the MQTT client task to handle the
			 * disconnection.
			 */
			if (status_cb) {
				status_cb(IOTC_CS_MQTT_DISCONNECTED);
			}
			break;
		}

		case CY_MQTT_EVENT_TYPE_SUBSCRIPTION_MESSAGE_RECEIVE: {
			cy_mqtt_publish_info_t *received_msg;


			/* Incoming MQTT message has been received. Send this message to
			 * the subscriber callback function to handle it.
			 */
			received_msg = &(event.data.pub_msg.received_message);
			if (mqtt_inbound_msg_cb && !is_disconnect_requested) {
				// we must ensure that this is a null-terminated string here
				char * topic_str = malloc(received_msg->topic_len + 1);
				if (!topic_str) {
					printf("Out of memory while trying to allocate the topic string!\n");
					break;
				}
				memcpy(topic_str, received_msg->topic, received_msg->topic_len);
				topic_str[received_msg->topic_len] = 0; // terminate it
				mqtt_inbound_msg_cb(received_msg->topic, received_msg->payload, received_msg->payload_len);
				free(topic_str);
			}
			is_disconnect_requested = false;
			break;
		}
		default: {
			/* Unknown MQTT event */
			printf("Unknown Event received from MQTT callback!\n");
			break;
		}
    }
}

static cy_rslt_t mqtt_subscribe(IotclMqttConfig *mc, cy_mqtt_qos_t qos) {
    /* Status variable */

    cy_mqtt_subscribe_info_t subscribe_info = {
    		.qos = qos, //
			.topic = mc->sub_c2d, //
			.topic_len = strlen(mc->sub_c2d) //
    };

    cy_rslt_t result = 1;

    /* Subscribe with the configured parameters. */
    for (uint32_t retry_count = 0; retry_count < MAX_SUBSCRIBE_RETRIES; retry_count++) {
        result = cy_mqtt_subscribe(mqtt_connection, &subscribe_info, 1);
        if (result == CY_RSLT_SUCCESS) {
            printf("MQTT client subscribed to the topic '%s' successfully.\n", subscribe_info.topic);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(MQTT_SUBSCRIBE_RETRY_INTERVAL_MS));
    }
    return result;
}

static cy_rslt_t mqtt_connect(IotclMqttConfig *mc) {
    /* Variable to indicate status of various operations. */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    cy_mqtt_connect_info_t connection_info = { //
    		.client_id = mc->client_id, //
			.client_id_len = strlen(mc->client_id), //
			.username = mc->username, //
			.username_len = mc->username ? strlen(mc->username) : 0, //
			.password = NULL, //
			.password_len = 0, //
			.clean_session = true, //
			.keep_alive_sec = 55, //
			.will_info = NULL //
	};

    /* Generate a unique client identifier with 'MQTT_CLIENT_IDENTIFIER' string
     * as a prefix if the `GENERATE_UNIQUE_CLIENT_ID` macro is enabled.
     */

    for (uint32_t retry_count = 0; retry_count < IOTC_MAX_MQTT_CONN_RETRIES; retry_count++) {

        /* Establish the MQTT connection. */
        result = cy_mqtt_connect(mqtt_connection, &connection_info);

        if (result == CY_RSLT_SUCCESS) {
            printf("MQTT connection successful.\n");
            return result;
        }

        printf("MQTT connection failed with error code 0x%08x. Retrying in %d ms. Retries left: %d\n", (int) result,
        IOTC_MQTT_CONN_RETRY_INTERVAL_MS, (int) (IOTC_MAX_MQTT_CONN_RETRIES - retry_count - 1));
        vTaskDelay(pdMS_TO_TICKS(IOTC_MQTT_CONN_RETRY_INTERVAL_MS));
    }

    printf("Exceeded maximum MQTT connection attempts\n");
    printf("MQTT connection failed after retrying for %d mins\n",
            (int) ((IOTC_MQTT_CONN_RETRY_INTERVAL_MS * IOTC_MAX_MQTT_CONN_RETRIES) / 60000u));
    return result;
}

static cy_rslt_t iotc_cleanup_mqtt() {
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_rslt_t ret = CY_RSLT_SUCCESS;
    is_connected = false;

    result = cy_mqtt_disconnect(mqtt_connection);
    if (result) {
        printf("Failed to disconnect the MQTT client. Error was:0x%08lx\n", result);
        is_disconnect_requested = false;
        ret = ret == CY_RSLT_SUCCESS ? result : CY_RSLT_SUCCESS;
    }
    is_connected = false;

    if (mqtt_connection) {
        result = cy_mqtt_delete(mqtt_connection);
        if (result) {
            printf("Failed to delete the MQTT client. Error was:0x%08lx\n", result);
        }
        mqtt_connection = NULL;
        ret = ret == CY_RSLT_SUCCESS ? result : CY_RSLT_SUCCESS;

    }
    if (is_mqtt_initialized) {
        result = cy_mqtt_deinit();
        if (result) {
            printf("Failed to deinit the MQTT client. Error was:0x%08lx\n", result);
        }
        is_mqtt_initialized = false;
        ret = ret == CY_RSLT_SUCCESS ? result : CY_RSLT_SUCCESS;
    }
    mqtt_inbound_msg_cb = NULL;
    status_cb = NULL;
    return ret;
}

cy_rslt_t iotc_mqtt_client_disconnect() {
    is_disconnect_requested = true;
    return iotc_cleanup_mqtt();
}

bool iotc_mqtt_client_is_connected() {
    return is_connected;
}

cy_rslt_t iotc_mqtt_client_publish(const char* topic, const char *payload, int qos) {
    /* Status variable */
    cy_rslt_t result;

    /* Structure to store publish message information. */
    cy_mqtt_publish_info_t publish_info = { //
    		.qos = (cy_mqtt_qos_t) qos, //
			.topic = topic, //
			.topic_len = strlen(topic), //
			.retain = false, //
			.dup = false //
    };

    /* Publish the data received over the message queue. */
    publish_info.payload = payload;
    publish_info.payload_len = strlen(payload);

    result = cy_mqtt_publish(mqtt_connection, &publish_info);

    if (result != CY_RSLT_SUCCESS) {
        printf("  Publisher: MQTT Publish failed with error 0x%0X.\n", (int) result);
        return result;
    }
    return CY_RSLT_SUCCESS;
}

cy_rslt_t iotc_mqtt_client_init(IotConnectMqttConfig *c) {
    /* Variable to indicate status of various operations. */
    cy_rslt_t result;

    IotclMqttConfig* mc = iotcl_mqtt_get_config();
    if (!mc) {
    	return CY_RSLT_MODULE_MQTT_ERROR; // called function will print the error
    }

    if (!mc->sub_c2d)

    mqtt_inbound_msg_cb = NULL;
    status_cb = NULL;
    is_connected = false;
    is_disconnect_requested = false;

    /* Initialize the MQTT library. */
    result = cy_mqtt_init();
    if (result) {
        iotc_cleanup_mqtt();
        printf("Failed to initialize the MQTT library. Error was:0x%08lx\n", result);
        return result;
    }

    cy_mqtt_broker_info_t broker_info = { //
    		.hostname = mc->host, //
			.hostname_len = strlen(mc->host),
			.port = 8883 //
    };

    cy_awsport_ssl_credentials_t security_info = { 0 };

    security_info.sni_host_name = mc->host;
    security_info.sni_host_name_size = strlen(mc->host) + 1; // yes, +1 !

    if (c->x509_config->server_ca_cert) {
    	security_info.root_ca = c->x509_config->server_ca_cert;
    } else {
    	switch(c->connection_type) {
    	case IOTC_CT_AWS:
    		security_info.root_ca = IOTCL_AMAZON_ROOT_CA1;
    		break;
    	case IOTC_CT_AZURE:
    		security_info.root_ca = IOTCL_CERT_DIGICERT_GLOBAL_ROOT_G2;
    		break;
    	default:
    		// the SDK will check, but just in case
    		printf("connection_type must be set Azure or AWS\n");
        	return CY_RSLT_MODULE_MQTT_BADARG;
    	}
    }
    security_info.root_ca_size = strlen(security_info.root_ca) + 1;

	security_info.client_cert = c->x509_config->device_cert;
	security_info.client_cert_size = strlen(c->x509_config->device_cert) + 1;
	security_info.private_key = c->x509_config->device_key;
	security_info.private_key_size = c->x509_config->device_key ? strlen(c->x509_config->device_key) + 1 : 0;

    /* Create the MQTT client instance. */
    result = cy_mqtt_create(
    		mqtt_network_buffer, MQTT_NETWORK_BUFFER_SIZE, //
			&security_info, &broker_info, //
			MQTT_HANDLE_DESCRIPTOR, //
			&mqtt_connection //
    );

    if (result) {
        printf("Failed to create the MQTT client. Error was:0x%08lx\n", result);
        iotc_cleanup_mqtt();
        return result;
    }

	/* Register a MQTT event callback */
	result = cy_mqtt_register_event_callback( mqtt_connection, (cy_mqtt_callback_t)mqtt_event_callback, NULL );
    if (result) {
        printf("Failed to register the MQTT callback! Error was:0x%08lx\n", result);
        iotc_cleanup_mqtt();
        return result;
    }

    result = mqtt_connect(mc);
    if (result) {
        iotc_cleanup_mqtt();
        return result;
    }
    result = mqtt_subscribe(mc, (cy_mqtt_qos_t) 1);
    if (result) {
        printf("Failed to subscribe to the MQTT topic. Error was:0x%08lx\n", result);
        iotc_cleanup_mqtt();
        return result;
    }
    is_connected = true;
    status_cb = c->status_cb;
    mqtt_inbound_msg_cb = c->mqtt_inbound_msg_cb;
    return result;
}
