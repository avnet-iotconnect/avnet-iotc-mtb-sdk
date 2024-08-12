# Introduction

The SDK can provide seamless AWS Device Qualification process with IoTConnect for any Infineon 
board that can use this SDK.

Please contact sales representative to learn more about AWS Device Qualification.

# Instructions

In order to qualify your device, you will need to coordinate with your sales representative 
to coordinate this test with the IoTConnect team.

Once you are ready to execute the test:
* Search for "DEFINES+=" in yor application Makefile and append this line to the list of defines that 
will be added to the application:

```
DEFINES+=IOTC_AWS_DEVICE_QUALIFICATION
```
* Ensure to add the command *aws-qualification-start* to your device's template. 
* Build, run the application, and ensure that the device is connected to your AWS IoTConnect account.
* Before actually initiating the qualification test, coordinate with our IoTConnect contact to obtain
the qualification endpoint.
* Execute the *aws-qualification-start* to your device with the qualification endpoint as the argument.
* Once the device receives the command, you should see it attempting to connect to the qualification endpoint.
* Engineer can proceed to run the qualification test in AWS Console at this time.
* Once the qualification test completes, power off or restart your device.