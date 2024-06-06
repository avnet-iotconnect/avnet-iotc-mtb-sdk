/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTC_MQTT_MQ_H
#define IOTC_MQTT_MQ_H

#include "cy_result.h"
#include "cyabs_rtos.h" 	// for cy_time_t
#include "iotc_mqtt_client.h" 	// for IotConnectMqttInboundMessageCallback
#include "iotconnect.h"

#ifdef __cplusplus
extern "C" {
#endif

cy_rslt_t iotc_mq_init(size_t queue_size);

void iotc_mq_register(IotConnectMqttInboundMessageCallback mqtt_inbound_msg_cb);

void iotc_mq_on_mqtt_inbound_message(const char* topic, const char *message, size_t message_len);

// Wait up to timeout_ms milliseconds, and if messages are available, processes them with itc-c-lib
// If timeout_ms is zero, the call will block forever until a message arrives
void iotc_mq_process(cy_time_t timeout_ms);

void iotc_mq_deregister(void);

void iotc_mq_flush(void);

void iotc_mq_deinit(void);

#ifdef __cplusplus
}
#endif


#endif // IOTC_MQTT_MQ_H
