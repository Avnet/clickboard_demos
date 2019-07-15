AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
ACLOCAL_ARFLAGS = cr

AM_CPPFLAGS = \
           -I. \
           -I../../clickmodules/LSM6DSL \
           -O2 -Wall -static -fno-short-enums \
           -Wl,--unresolved-symbols=ignore-in-shared-libs 

AM_LDFLAGS = -static -lhw 

AM_CXXFLAGS = -std=c++14 -std=gnu++11 

clean-generic:
	-$(MAKE) 

bin_PROGRAMS = lsm6dsl_demo

lsm6dsl_demo_SOURCES = lsm6dsl_demo.c 
lsm6dsl_demo_LDADD = libclick.a 

noinst_LIBRARIES = libclick.a 

libclick_a_SOURCES = ../../clickmodules/LSM6DSL/LSM6DSL_ACC_GYRO_Driver.c \
                     ../../clickmodules/LSM6DSL/LSM6DSLSensor.c \
                     ../../clickmodules/LSM6DSL/lsm6dsl.c
