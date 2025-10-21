This repository contains the /IOTCONNECT ModusToolbox SDK for use with Infineon PSOC6, PSOC EDGE and similar products.

Refer to the [PSOC6 Basic Example](https://github.com/avnet-iotconnect/avnet-iotc-mtb-basic-example) project 
for a reference PSOC6 implementation and OTA support.

Refer to the [PSOC Edge Baby Monitor](https://github.com/avnet-iotconnect/avnet-iotc-mtb-psoc-edge-baby-monitor) project 
for a reference PSOC Edge implementation.

This code is primarily based on the 
[mtb-example-wifi-mqtt-client](https://github.com/Infineon/mtb-example-wifi-mqtt-client)
project on PSOC6, and the
[mtb-example-psoc-edge-wifi-mqtt-client](https://github.com/Infineon/mtb-example-anycloud-mqtt-client)
project on PSOC Edge.

## Dependencies

The project uses the following dependent projects as git submodules:
* [iotc-c-lib](https://github.com/avnet-iotconnect/iotc-c-lib.git) v3.1.0-proto-v2.1

Additionally, the project importing the SDK, should import the following libraries:
* [mqtt](https://github.com/Infineon/mqtt):
MQTT support (tested with v4.7.0)
* [http-client](https://github.com/Infineon/http-client):
HTTPS Support (tested with v1.8.1)
* [retarget-io](https://github.com/Infineon/retarget-io):
Stdio redirect to UART (tested with v1.8.1)
* [wifi-core-freertos-lwip-mbedtls](https://github.com/Infineon/wifi-core-freertos-lwip-mbedtls):
WiFi and MbedTLS with FreeRTOS - version 2.X only supported by PSOC6 (tested with v3.1.0 on PSOC Edge, v2.2.1 on PSOC6) 

## Contributing To This Project 

When contributing to this project, please follow the contributing guidelines for 
[iotc-c-lib](https://github.com/avnet-iotconnect/iotc-c-lib/blob/master/CONTRIBUTING.md) 
where applicable and possible.
