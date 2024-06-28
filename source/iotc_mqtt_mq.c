/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "iotcl.h"
#include "iotcl_util.h"
#include "iotc_mqtt_mq.h"

#ifndef IOTC_MQ_PUT_TIMEOUT
#define IOTC_MQ_PUT_TIMEOUT 100
#endif

typedef struct IotcMqMessage {
	char *topic;
	char *message;
	size_t message_len;
} IotcMqMessage;

static cy_queue_t cy_queue = NULL;
static bool is_initialized = false;

static IotConnectMqttInboundMessageCallback client_msg_cb = NULL;

static void iotc_mq_destroy_message(IotcMqMessage *msg) {
	if (msg->topic) {
		iotcl_free(msg->topic);
		msg->topic = NULL;
	}
	if (msg->message) {
		iotcl_free(msg->message);
		msg->message = NULL;
	}
	msg->message_len = 0;
}

static bool iotc_mq_create_message(IotcMqMessage *msg, const char* topic, const char *message, size_t message_len) {
	msg->topic = iotcl_strdup(topic);
	msg->message = iotcl_malloc(message_len);
	if (!msg->topic || !msg->message) {
		printf("ERROR: iotc_mq: Out of memory while allocating a queue message\n");
		iotc_mq_destroy_message(msg);
		return false;
	}
	memcpy(msg->message, message, message_len);
	msg->message_len = message_len;
	return true;
}


cy_rslt_t iotc_mq_init(size_t queue_size) {
	cy_rslt_t result;
    result = cy_rtos_init_queue(&cy_queue, queue_size, sizeof(IotcMqMessage));
    if (CY_RSLT_SUCCESS != result) {
    	printf("ERROR: iotc_mq_init queue error 0x%lx.\n", CY_RSLT_GET_CODE(result));
    }
    is_initialized = true;
    return result;
}

void iotc_mq_register(IotConnectMqttInboundMessageCallback mqtt_inbound_msg_cb) {
	client_msg_cb = mqtt_inbound_msg_cb;
}

void iotc_mq_on_mqtt_inbound_message(const char* topic, const char *message, size_t message_len) {
    cy_rslt_t result;
    IotcMqMessage msg;

    if (!topic || !message || 0 == message_len) {
    	printf("ERROR: iotc_mq: Internal error! Topic, message or message length are invalid !\n");
    	return;
    }

    if (!client_msg_cb) {
    	printf("ERROR: iotc_mq: Received a message, but no callback registered!\n");
    	return;
    }

    if (false == iotc_mq_create_message(&msg, topic, message, message_len)) {
    	return; // called function will print the error. We just need to return.
    }

    // we should be able to put the message in immediately, so give it a rough timeout
    result = cy_rtos_put_queue(&cy_queue, &msg, IOTC_MQ_PUT_TIMEOUT, false);
    if (CY_RSLT_SUCCESS != result) {
    	iotc_mq_destroy_message(&msg);
    	printf("ERROR: iotc_mq: queue put error 0x%lx.\n", CY_RSLT_GET_CODE(result));
    }
}

void iotc_mq_process(cy_time_t timeout_ms) {
	IotcMqMessage msg;
	if (!client_msg_cb) {
		printf("WARN: iotc_mq_process: No callback registered!\n");
		return;
	}

	cy_rslt_t result;
	do {
		result = cy_rtos_get_queue(&cy_queue, (void *)&msg, timeout_ms, false );
	    if (result == CY_RTOS_QUEUE_EMPTY) {
	    	// should should be able to break early here, but never happens .. see comments below
	    	return;
	    }

	    if (result == CY_RSLT_SUCCESS) {
			client_msg_cb(msg.topic, msg.message, msg.message_len);
	    } else {
	    	// Seems that with this case https://github.com/Infineon/freertos/blob/release-v10.5.002/Source/queue.c#L1494
	    	// there is no return from the queue, so we have to do some shenanigans here...
	    	size_t num_waiting;
	    	result = cy_rtos_count_queue(&cy_queue, &num_waiting);
	    	if (result == CY_RSLT_SUCCESS) {
	    		if (0 != num_waiting) {
			        printf("Got error 0x%x while getting messages from the message queue\n", (unsigned int)result);
	    		} // else it's all good. We indeed timed out
	    	} else {
		        printf("cy_rtos_get_num_waiting error 0x%x\n", (unsigned int)result);
	    	}
	    	return; // in either case
	    }
	} while (result == CY_RSLT_SUCCESS);
}

void iotc_mq_flush(void) {
	IotcMqMessage msg;

	memset(&msg, 0, sizeof(IotcMqMessage));

	// If we give 0 timeout, it will never return after the last one, so use at least 1 so that it doesn't block indefinitely
	while(CY_RSLT_SUCCESS == cy_rtos_get_queue( &cy_queue, (void *)&msg, 1, false )) {
		iotc_mq_destroy_message(&msg);
	}
}

void iotc_mq_deregister(void) {
	iotc_mq_flush();
	client_msg_cb = NULL;
}

void iotc_mq_deinit(void) {
	iotc_mq_deregister();
	iotc_mq_flush();
	if (is_initialized) {
		is_initialized = false;
		cy_rslt_t result = cy_rtos_deinit_queue(&cy_queue);
	    if (CY_RSLT_SUCCESS != result) {
	    	printf("ERROR: iotc_mq_init queue error 0x%lx.\n", CY_RSLT_GET_CODE(result));
	    }
	}
}

