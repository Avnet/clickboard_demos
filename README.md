---

# Click Board demonstration code
## Overview
This repository contains example solutions for using the vairious click modules. 

## Test Platform examples 
### AT&T Starter Kit 2 using the LTE IoT Breakout Carrier (ATTSK2 sub-directory). 

#### To Build and Execute the demo programs:

1.  Install the WNC-SDK
2.  Add the environment variables ****" . /usr/local/oecore-x86_64/environment-setup-cortexa7-neon-vfpv4-oe-linux-gnueabi "****  _(there is a spaces after the '.')
3.  run autogen: **./autogen.sh**
4.  run configure: **./configure ${CONFIGURE_FLAGS}**
5.  make the programs: “**make**”
    
Three executables are generated: ‘cdemo’ (an example using C), ‘cppdemo’ (same example using C++), and tof_demo (Time Of Flight using LightRanger/VL53L0X Click)

The test programs each utilize libaries that are created from the common sensor code located in the parent directory.  Code  located in the sub-directories is specific to the platform they are built for.  Libraries for the Click modules {libclick.a for the Barometer, Temp&Humi, OLED-B; libvl53l0x.a for the LightRanger Click) are then linked with when creating the example programs.  The LightRanger Click is in a unique library because we are re-using available ST code (https://www.st.com/en/imaging-and-photonics-solutions/vl53l0x.html) for controlling the VL53L0X. 

##### cdemo 

The 'C' demo program consists of:
1. c_demo.c     - Main program and supporting functions such as I2C and SPI routines
2. oled_test.c  - Drives a series of OLED tests
3. libclick.a   - Library containing code for Click modules

The 'C++' demo program consists of:
1. demo.cpp           - Main program
2. oled_test.c        - Drives a series of OLED tests
3. i2c_interface.cpp  - class for managing I2C
4. oledb.cpp          - class for managing the OLED-B

The 'tof_demo' program is implemented only in 'C' because it is using a 'C' library that ST provides.  By re-using the ST code, all supporting ST documentation maintains its relavence and accuracy.  The tof_demo program consists of:
1. tof_demo.c          - Main program which drives 5 different ranging algorithms
2. vl53l0x_i2c.c       - The I2C routines needed to interact with the Click Board

Push each to the ATT SK2 and executed in an adb shell.


