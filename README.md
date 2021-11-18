## About

This repository contains the IoTConnect ModusToolbox SDK and code samples that make use of the SDK.

This code is based on the [mtb-example-anycloud-mqtt-client](https://github.com/Infineon/mtb-example-anycloud-mqtt-client) project 
 and indirectly on the Infineon's [MQTT 3.1.1](https://github.com/Infineon/mqtt) client library.

## Supported Boards

The code has been developed and tested With:
- [PSoC&trade; 6 Wi-Fi Bluetooth&reg; pioneer kit](https://www.cypress.com/CY8CKIT-062-WiFi-BT) (`CY8CKIT-062-WIFI-BT`)

The following boards *should* work without any code modification, but have not been tested:
- [PSoC&trade; 6 Wi-Fi Bluetooth&reg; prototyping kit](https://www.cypress.com/CY8CPROTO-062-4343W) (`CY8CPROTO-062-4343W`) - Default value of `TARGET`
- [PSoC&trade; 62S2 Wi-Fi Bluetooth&reg; pioneer kit](https://www.cypress.com/CY8CKIT-062S2-43012) (`CY8CKIT-062S2-43012`)
- [PSoC&trade; 62S1 Wi-Fi Bluetooth&reg; pioneer kit](https://www.cypress.com/CYW9P62S1-43438EVB-01) (`CYW9P62S1-43438EVB-01`)
- [PSoC&trade; 62S1 Wi-Fi Bluetooth&reg; pioneer kit](https://www.cypress.com/CYW9P62S1-43012EVB-01) (`CYW9P62S1-43012EVB-01`)
- [PSoC&trade; 62S2 evaluation kit](https://www.cypress.com/CY8CEVAL-062S2) (`CY8CEVAL-062S2-LAI-4373M2`)


## Build Instructions

- Download and extract the project package from [Releases](releases/)
- Download and open [ModusToolbox&trade; software](https://www.cypress.com/products/modustoolbox-software-environment) v2.2 or later (tested with v2.3)
- Select a name for your workspace when prompted for a workspace name.
- Click the **New Application** link in the **Quick Panel** (or, use **File** > **New** > **ModusToolbox Application**). This launches the [Project Creator](https://www.cypress.com/ModusToolboxProjectCreator) tool.
- Pick a kit supported by the code example from the list shown in the **Project Creator - Choose Board Support Package (BSP)** dialog and click **Next**
- In the **Project Creator - Select Application** dialog, click on the **Import** button and 
   navigate to the clone of this repo or downloaded package's **sampple/basic-sample** directory and click **Open**. 
- Click the checkbox next to the **basic-sample** application in the list and chick **Create**.
- Modify samples/basic-sample/configs/app_config.h per your IoTConnect device and account info.
- At this point you should be able to build and run the application by using the options in the **Quick Panel** on bottom left of the screen.   
- You should see the application output in your terminal emulator.

NOTE: If you cloned the repo, note that the SDK will be pulled from this GitHub URL and modifying the contents of iotc-modustoolbox-sdk will have no effect on your code execution. 
If you need to make temporary modifications to the SDK, you can modify the contents in the mtb_shared directory.