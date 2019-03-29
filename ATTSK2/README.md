
# AT&T Starter Kit 2 using the LTE IoT Breakout Carrier
This code assumes you are using a http://cloudconnectkits.org/product/lte-starter-kit-2 in conjunction with http://cloudconnectkits.org/product/lte-iot-breakout-carrier.

## Build and Execute the demo programs:
In all subdirectories, the following steps must be performed to build the executable image.

1.  Install the WNC-SDK
2.  Add the environment variables 
****" . /usr/local/oecore-x86_64/environment-setup-cortexa7-neon-vfpv4-oe-linux-gnueabi "****  _(there is a spaces after the '.')_
3.  run autogen: **./autogen.sh**
4.  run configure: **./configure ${CONFIGURE_FLAGS}**
5.  make the program: “**make**”

After **make** is executed, an executable image will be generated (the name corresponds to the name of the sub-directory).

## Demo Programs
The code located in the sub-directories is specific to the platform and demo they are built for.  In all cases, the 
code from the clickmodule library (https://github.com/jflynn129/clickmodule) is built into a linkable library with 
the demo specific code the linked to it (*for details on the build, see Makefile.am*).
After the executable image has been generated, push it tothe CUSTAPP directory (**adb push XYZ /CUSTAPP/**) and execute 
it (**adb shell "/CUSTAPP/XYZ"**) in an adb shell.

1. click_demo
Two executable images are generated: ‘_cdemo_’ (C example), ‘_cppdemo_’ (C++ example). 

* _cdemo_ (the 'C' example) consists of the following files:
  * c_demo.c     - Main program and supporting functions such as I2C and SPI routines
  * oled_test.c  - Drives a series of OLED tests
  * libclick.a   - Library containing code for Click modules

* _cppdemo_ (the 'C++' example) consists of:
  * demo.cpp           - Main program
  * oled_test.c        - Drives a series of OLED tests
  * i2c_interface.cpp  - class for managing I2C
  * oledb.cpp          - class for managing the OLED-B

2. flame_demo

3. hr4_demo

5. relay_demo

6. tof_demo
The LightRanger Click is in a unique library because we are re-using available ST code (https://www.st.com/en/imaging-and-photonics-solutions/vl53l0x.html) for controlling the VL53L0X.

The 'tof_demo' program is implemented only in 'C' because it is using a 'C' library that ST provides.  By re-using the ST code, all supporting ST documentation maintains its relavence and accuracy.  The tof_demo program consists of:
1. tof_demo.c          - Main program which drives 5 different ranging algorithms
2. vl53l0x_i2c.c       - The I2C routines needed to interact with the Click Board
