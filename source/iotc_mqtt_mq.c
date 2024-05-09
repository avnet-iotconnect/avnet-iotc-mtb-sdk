#include <stdbool.h>
#include <string.h>
#include "cyabs_rtos.h"
#include "iotcl.h"
#include "iotc_mqtt_mq.h"

#ifndef IOTC_MQ_PUT_TIMEOUT
#define IOTC_MQ_PUT_TIMEOUT 100
#endif

typedef struct IotcMqMessage {
	const char* topic;
	const char *message;
	size_t message_len;
} IotcMqMessage;
static cy_queue_t        mqtt_event_queue = NULL;

static IotConnectMqttInboundMessageCallback client_msg_cb = NULL;

static void iotc_mq_destroy_message(const IotcMqMessage *msg) {
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

static bool iotc_mq_create_message(const IotcMqMessage *msg, const char* topic, const char *message, size_t message_len) {
	msg->topic = iotcl_strdup(topic);
	msg->message = iotcl_malloc(message_len);
	if (!msg->topic || !msg->message) {
		printf("ERROR: iotc_mq: Out of memory while allocating a queue message\n");
		iotc_mq_destroy_message(msg);
	}
	memcpy(msg->message, message, message_len);
	msg->message_len = message_len;
}


cy_rslt_t result iotc_mq_init(size_t queue_size) {
	cy_rslt_t result;
    result = cy_rtos_init_queue(&mqtt_event_queue, queue_size, sizeof(IotcMqMessage));
    if (CY_RSLT_SUCCESS != result) {
    	printf("ERROR: iotc_mq_init queue error 0x%lx.\n", CY_RSLT_GET_CODE(result));
    }
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
    }

    if (!iotc_mq_create_message(&msg, topic, message, message_len)) {
    	return; // called function will print the error. We just need to return.
    }

    // we should be able to put the message in immediately, so give it a rough timeout
    result = cy_rtos_put_queue(&mqtt_event_queue, &msg, IOTC_MQ_PUT_TIMEOUT, false);
    if (CY_RSLT_SUCCESS != result) {
    	printf("ERROR: iotc_mq: queue put error 0x%lx.\n", CY_RSLT_GET_CODE(result));
    }
}

void iotc_mq_process(unsigned int timeout_ms) {
	IotcMqMessage msg;
	if (!client_msg_cb) {
		printf("WARN: iotc_mq_process: No callback registered!\n");
		return;
	}

    result = cy_rtos_get_queue( &mqtt_event_queue, (void *)&socket_event, CY_RTOS_NEVER_TIMEOUT, false );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_mqtt_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\ncy_rtos_get_queue failed with Error :[0x%X]\n", (unsigned int)result );
        continue;
    }
	client_msg_cb(msg.topic, msg.message, msg.message_len);

void iotc_mq_flush(void) {
    cy_rslt_t result;
	IotcMqMessage msg;

	memset(&msg, 0, sizeof(IotcMqMessage));

	while( CY_RSLT_SUCCESS == (result = cy_rtos_get_queue( &mqtt_event_queue, (void *)&socket_event, CY_RTOS_NEVER_TIMEOUT, false ))) {
		iotc_mq_destroy_message(&msg);
	}
}

void iotc_mq_deregister(void) {
	iotc_mq_flush();
	client_msg_cb = NULL;
}

void iotc_mq_deinit(void) {
	iotc_mq_deregister();
    result = cy_rtos_deinit_queue(&mqtt_event_queue);
    if (CY_RSLT_SUCCESS != iotc_mq_deinit) {
    	printf("ERROR: iotc_mq_init queue error 0x%lx.\n", CY_RSLT_GET_CODE(result));
    }

}

