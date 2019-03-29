AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
ACLOCAL_ARFLAGS = cr

AM_CPPFLAGS = -I. -I../../../clickmodule/LIGHTRANGER/ \
           -O2 -Wall -static -fno-short-enums \
           -Wl,--unresolved-symbols=ignore-in-shared-libs 

AM_LDFLAGS = -static -lhw 

AM_CXXFLAGS = -std=c++14 -std=gnu++11 

clean-generic:
	-$(MAKE) 

bin_PROGRAMS = tof_demo 

noinst_LIBRARIES = libvl53l0x.a

libvl53l0x_a_CFLAGS = \
           -I../../../clickmodule/LIGHTRANGER/core/inc \
           -I../../../clickmodule/LIGHTRANGER/platform/inc \
           -Wall -fmessage-length=0 \
           -O2 -Wall -static -fno-short-enums \
           -Wl,--unresolved-symbols=ignore-in-shared-libs \
           -DTRUE=1 -DFALSE=0 -DUSE_I2C_2V8

libvl53l0x_a_SOURCES = ../../../click/LIGHTRANGER/core/src/vl53l0x_api_calibration.c \
    ../../../clickmodule/LIGHTRANGER/core/src/vl53l0x_api_core.c \
    ../../../clickmodule/LIGHTRANGER/core/src/vl53l0x_api_ranging.c \
    ../../../clickmodule/LIGHTRANGER/core/src/vl53l0x_api_strings.c \
    ../../../clickmodule/LIGHTRANGER/core/src/vl53l0x_api.c \
    ../../../clickmodule/LIGHTRANGER/platform/src/vl53l0x_platform.c

tof_demo_CFLAGS = $(libvl53l0x_a_CFLAGS)
tof_demo_SOURCES = tof_demo.c vl53l0x_i2c.c 
tof_demo_LDADD = libvl53l0x.a 

