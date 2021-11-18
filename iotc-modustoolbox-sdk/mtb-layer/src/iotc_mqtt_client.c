/*******************************************************************************
 * Copyright 2020-2021, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 *******************************************************************************/
//
// Copyright: Avnet 2021
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 11/11/21.
//
#include "cyhal.h"
#include "cybsp.h"

/* FreeRTOS header files */
#include "FreeRTOS.h"
#include "task.h"

/* Middleware libraries */
#include "cy_retarget_io.h"
#include "cy_lwip.h"
#include "cy_mqtt_api.h"

#include "cy_mqtt_api.h"
#include "clock.h"

/* LwIP header files */
#include "lwip/netif.h"
#include "iotconnect_certs.h"
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

static cy_mqtt_t mqtt_connection;
static uint8_t mqtt_network_buffer[MQTT_NETWORK_BUFFER_SIZE];
static char *publish_topic = NULL; // pointer to sync response's publish topic. Should be available as long as we are connected.
static bool is_connected = false;
static bool is_disconnect_requested = false;
static bool is_mqtt_initialized = false;
static IotConnectC2dCallback c2d_msg_cb = NULL; // callback for inbound messages
static IotConnectStatusCallback status_cb = NULL; // callback for connection status

static void mqtt_event_callback(cy_mqtt_t mqtt_handle, cy_mqtt_event_t event, void *user_data) {
    cy_mqtt_publish_info_t *received_msg;

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

        /* Incoming MQTT message has been received. Send this message to
         * the subscriber callback function to handle it.
         */
        received_msg = &(event.data.pub_msg.received_message);
        if (c2d_msg_cb && !is_disconnect_requested) {
            c2d_msg_cb(received_msg->payload, received_msg->payload_len);
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

static cy_rslt_t mqtt_subscribe(IotConnectMqttConfig *mqtt_config, cy_mqtt_qos_t qos) {
    /* Status variable */

    cy_mqtt_subscribe_info_t subscribe_info = { .qos = qos, .topic = mqtt_config->sr->broker.sub_topic, .topic_len =
            strlen(mqtt_config->sr->broker.sub_topic) };

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

static cy_rslt_t mqtt_connect(IotConnectMqttConfig *mqtt_config) {
    /* Variable to indicate status of various operations. */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    cy_mqtt_connect_info_t connection_info = { //
            .client_id = mqtt_config->sr->broker.client_id, //
                    .client_id_len = strlen(mqtt_config->sr->broker.client_id), //
                    .username = mqtt_config->sr->broker.user_name, //
                    .username_len = strlen(mqtt_config->sr->broker.user_name), //
                    .password = NULL, //
                    .password_len = 0, //
                    .clean_session = true, //
                    .keep_alive_sec = 60, //
                    .will_info = NULL //
            };

    /* NOTE: Symmetric key not supported yet */
    if (mqtt_config->auth->type == IOTC_AT_TOKEN || mqtt_config->auth->type == IOTC_AT_SYMMETRIC_KEY) {
        connection_info.password = mqtt_config->sr->broker.pass;
        connection_info.password_len = strlen(mqtt_config->sr->broker.pass);
    }

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
            (int) (IOTC_MQTT_CONN_RETRY_INTERVAL_MS * IOTC_MAX_MQTT_CONN_RETRIES) / 60000u);
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
    publish_topic = NULL;
    c2d_msg_cb = NULL;
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

cy_rslt_t iotc_mqtt_client_publish(const char *payload, int qos) {
    /* Status variable */
    cy_rslt_t result;

    if (!publish_topic) {
        printf("iotc_mqtt_client_publish: MQTT is not connected\n");
        return (cy_rslt_t) 1;
    }

    /* Structure to store publish message information. */
    cy_mqtt_publish_info_t publish_info = { .qos = (cy_mqtt_qos_t) qos, .topic = publish_topic, .topic_len = strlen(
            publish_topic), .retain = false, .dup = false };

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

    if (publish_topic) {
        printf("WARNING: MQTT client initialized without disconnecting?\n");
        free(publish_topic);
    }
    publish_topic = NULL;
    c2d_msg_cb = NULL;
    status_cb = NULL;
    is_connected = false;
    is_disconnect_requested = false;

    /* Initialize the MQTT library. */
    result = cy_mqtt_init();
    if (result) {
        iotc_cleanup_mqtt();
        printf("Failed to intialize the MQTT library. Error was:0x%08lx\n", result);
        return result;
    }

    cy_mqtt_broker_info_t broker_info = { //
            .hostname = c->sr->broker.host, //
                    .hostname_len = strlen(c->sr->broker.host), .port = 8883 //
            };

    cy_awsport_ssl_credentials_t security_info = { 0 };
    security_info.root_ca = CERT_BALTIMORE_ROOT_CA;
    security_info.root_ca_size = sizeof(CERT_BALTIMORE_ROOT_CA);

    // mqtt_connect handles different auth types
    if (c->auth->type == IOTC_AT_X509) {
        security_info.client_cert = c->auth->data.cert_info.device_cert;
        security_info.client_cert_size = strlen(c->auth->data.cert_info.device_cert) + 1;
        security_info.private_key = c->auth->data.cert_info.device_key;
        security_info.private_key_size = strlen(c->auth->data.cert_info.device_key) + 1;
    }

    /* Create the MQTT client instance. */
    result = cy_mqtt_create(mqtt_network_buffer, MQTT_NETWORK_BUFFER_SIZE, &security_info, &broker_info,
            (cy_mqtt_callback_t) mqtt_event_callback, NULL, &mqtt_connection);

    if (result) {
        printf("Failed to create the MQTT client. Error was:0x%08lx\n", result);
        iotc_cleanup_mqtt();
        return result;
    }

    result = mqtt_connect(c);
    if (result) {
        iotc_cleanup_mqtt();
        return result;
    }
    result = mqtt_subscribe(c, (cy_mqtt_qos_t) 1);
    if (result) {
        printf("Failed to subscribe to the MQTT topic. Error was:0x%08lx\n", result);
        iotc_cleanup_mqtt();
        return result;
    }
    is_connected = true;
    publish_topic = c->sr->broker.pub_topic;
    status_cb = c->status_cb;
    c2d_msg_cb = c->c2d_msg_cb;
    return result;
}
