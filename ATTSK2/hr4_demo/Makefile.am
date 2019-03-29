AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
ACLOCAL_ARFLAGS = cr

AM_CFLAGS = -I. -I../../../clickmodule/HEARTRATE4/ \
           -O2 -Wall -static -fno-short-enums \
           -Wl,--unresolved-symbols=ignore-in-shared-libs 

AM_LDFLAGS = -static -lhw -lm

AM_CXXFLAGS = -std=c++14 -std=gnu++11 

clean-generic:

bin_PROGRAMS = hr4_demo

noinst_LIBRARIES   = libclick.a 
libclick_a_SOURCES = ../../../clickmodule/HEARTRATE4/max30102.c \
                     ../../../clickmodule/HEARTRATE4/algorithm_by_RF.c 

hr4_demo_source = hr4_demo.c
hr4_demo_LDADD = libclick.a 


