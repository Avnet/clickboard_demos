AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
ACLOCAL_ARFLAGS = cr

AM_CPPFLAGS = \
           -I. \
           -I../../../clickmodule/BAROMETER/ \
           -I../../../clickmodule/TEMP_HUM/ \
           -I../../../clickmodule/OLED-B/ \
           -O2 -Wall -static -fno-short-enums \
           -Wl,--unresolved-symbols=ignore-in-shared-libs 

AM_LDFLAGS = -static -lhw 

AM_CXXFLAGS = -std=c++14 -std=gnu++11 

clean-generic:
	-$(MAKE) 

bin_PROGRAMS = cpp_demo c_demo 

cpp_demo_SOURCES = cpp_demo.cpp i2c_interface.cpp oled_test.c oledb.cpp
cpp_demo_LDADD = libclick.a 

c_demo_SOURCES = c_demo.c oled_test.c
c_demo_LDADD = libclick.a 

noinst_LIBRARIES = libclick.a 

libclick_a_SOURCES = ../../../clickmodule/TEMP_HUM/hts221.c \
                     ../../../clickmodule/BAROMETER//barometer.c \
                     ../../../clickmodule/OLED-B/oledb_ssd1306.c \
                     ../../../clickmodule/OLED-B/Avnet_GFX.c 

