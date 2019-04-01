AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
ACLOCAL_ARFLAGS = cr

AM_CPPFLAGS = -I. -I../../clickmodule/RELAY/ \
           -O2 -Wall -static -fno-short-enums \
           -Wl,--unresolved-symbols=ignore-in-shared-libs 

AM_LDFLAGS = -static -lhw 

AM_CXXFLAGS = -std=c++14 -std=gnu++11 

clean-generic:
	-$(MAKE) 

bin_PROGRAMS = relay_demo 

noinst_LIBRARIES = libclick.a 

libclick_a_SOURCES = ../../clickmodule/RELAY/relay.c 

relay_demo_source = relay_demo.c
relay_demo_LDADD = libclick.a

