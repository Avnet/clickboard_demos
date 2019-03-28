# Overview
This code provides an example implementation of an Azure IoT Client using a Cellular data connection.  The code follows the style and content of the othere examples Microsoft provides (see https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-get-started-physical) but is taliored to use the [ARM Mbed OS](https://github.com/ARMmbed/mbed-os) (currently at v5.x) and the [ARM Easy-Connect](https://github.com/ARMmbed/easy-connect) framework. 

Generally speaking, there are two steps necessary to exercise this example code:
1. Setting up and configuring an Azure IoT Hub
2. Creating the IoT Client code and running it on the IoT End Device

This repository focues on step #2.  To learn more about step #1 refer to the [Element14 Web Site](https://www.element14.com/community/groups/mbed/blog/2018/09/21/implementing-an-azure-iot-client-using-the-mbed-os-part-1) blog post or any of the [Microsoft examples](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-get-started-physical).


# Hardware Used
The hardware platform used for this project consists of:

* STM32L4 Nucleo-144, Cortex-M4 development board
* Motion MEMS and environmental sensor board
* Avnet RSR1157 NBIoT BG96 Expansion Board

## Note
  The code used to develop this example utilized the following STM32 Nucleo-L496ZG and Quectel BG96 Module Firmware:

 -  BG96 Modem SW Revision: BG96 SW Revision: BG96 Rev:BG96MAR02A07M1G
 -  STM32L Version: 0221 / Build: May 30 2018 10:37:36

# Required Software Tools
1.  The mbed-cli is available from: [https://github.com/ARMmbed/mbed-cli](https://github.com/ARMmbed/mbed-cli)
2.  Latest version of GNU ARM Embedded Tool chain, available at: [**https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads**](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads)

# Building the samples:
To build the Sample program, you must use the mbed command line tools (mbed-cli). This is because the on-line tools do not allow you to modify the build parameters of the compiler but when using the CLI, you are able to specify a build profile to compile with.
###  Command Line Steps
1.  Import the [azure-iot-mbed-client](https://github.com/Avnet/azure-iot-mbed-client)  project:  
>     **mbed import  https://github.com/Avnet/azure-iot-mbed-client**

2. Edit the AvnetBG96_azure_client.cpp file and correctly set your connection string and device ID[^1] (lines #66 and #68):

    static const char* connectionString = "HostName=XX;DeviceId=xx;SharedAccessKey=xx";
    static const char* deviceId = "xxxx"; /*must match the one on connectionString*/

[^1]:	This information is available from your IoT Hub.
 

4.  Goto the azure-iot-mbed-client folder and ensure the ***mbed_settings.py*** file has the correct path to your compiler using GCC_ARM_PATH 

5.  Build the program by executing the command: 
>     **mbed compile -m NUCLEO_L496ZG -t GCC_ARM --profile toolchain_debug.json**

6.  The executable program will be located at: **BUILD/NUCLEO_L496ZG/GCC_ARM-TOOLCHAIN_DEBUG/azure-iot-mbed-client.bin** 

7.  Copy the executable program the Nucleo board that is connected to your PC, and once copied, press the reset button.
# Execution Output
When the program is run, you will see output similar to:
> 
> The example program interacts with Azure IoTHub sending sensor data
> and receiving messeages (using ARM Mbed OS v5.x) [using IKS01A2
> Environmental Sensor]
> 
> [EasyConnect] Using BG96  
> [EasyConnect] Connected to Network successfully  
> [EasyConnect] MAC address 19:95:91:41:02:72:20
> [EasyConnect] IP address 10.192.59.128  
> [ Platform  ] BG96 SW Revision: BG96 Rev:BG96MAR02A07M1G  
> [ Platform  ] Time set to Tue Sep 4 20:59:13 2018
> 
> (0001)Send IoTHubClient Message@20:59:13 - OK. [RSSI=99]  
> (0002)Send IoTHubClient Message@20:59:25 - OK. [RSSI=17]  
> (0003)Send IoTHubClient Message@20:59:31 - OK. [RSSI=17]  
> (0004)Send IoTHubClient Message@20:59:37 - OK. [RSSI=17]  
> (0005)Send IoTHubClient Message@20:59:43 - OK. [RSSI=17]

Each time the a message is sent to the Azure IoT Hub, it can be displayed using the ***iothub-explorer*** monitor-events  command.  When used, the output will resemble:

    {
      "ObjectName": "Avnet NUCLEO-L496ZG+BG96 Azure IoT Client",
      "ObjectType": "SensorData",
      "Version": "1.0",
      "ReportingDevice": "STL496ZG-BG96",
      "Temperature": 90.68,
      "Humidity": 54,
      "Pressure": 1008,
      "Tilt": 2,
      "ButtonPress": 0,
      "TOD": "Thu 2018-09-06 19:36:45 UTC"
    }
    ---- application properties ----
    {}
Time TOD stamp in the message will match the time displayed when the message is sent, e.g., the **Message@<<>>** will match **"TOD" : "<<>>"**

# Next steps
After You’ve run this sample application, collected sensor data, and interacted with your IoT hub, you may want to exercise some of the other Azure IoT scenarios.  See the following tutorials:

|Scenario                                  | Azure service or tool               |
|----------------------------------------  |:----------------------------------- |
|[Manage IoT Hub messages]( https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-explorer-cloud-device-messaging)                  | iothub-explorer tool                |
|[Manage IoT Hub messages](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-vscode-iot-toolkit-cloud-device-messaging)                   | VS Code Azure IoT Toolkit extension |
|[Manage your IoT device](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-device-management-iot-extension-azure-cli-2-0)                    | Azure CLI 2.0 and the IoT extension |
|[Manage your IoT device](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-device-management-iot-toolkit)                    | VS Code Azure IoT Toolkit extension |
|[Save IoT Hub messages to Azure storage]()    | Azure table storage                 |   
|[Visualize sensor data](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-live-data-visualization-in-power-bi)                     | Microsoft Power BI                  |
|[Visualize sensor data](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-live-data-visualization-in-web-apps)                     | Azure Web Apps                      |
|[Forecast weather with sensor data](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-weather-forecast-machine-learning)         | Azure Machine Learning              |
|[Automatic anomaly detection and reaction](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-monitoring-notifications-with-azure-logic-apps)  | Azure Logic Apps                    |




