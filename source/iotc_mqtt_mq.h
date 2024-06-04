#include "cy_result.h"
#include "iotconnect.h"

void iotc_mq_init(int queue_size);
void iotc_mq_register(IotConnectMqttInboundMessageCallback mqtt_inbound_msg_cb);

//  waits up to timeout_ms, and if message is available, processes a single message
void iotc_mq_process(unsigned int timeout_ms);

void iotc_mq_deregister(void);
void iotc_mq_flush(void);

void iotc_mq_deinit(void);
