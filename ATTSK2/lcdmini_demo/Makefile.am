AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
ACLOCAL_ARFLAGS = cr

AM_CPPFLAGS = \
           -I. \
           -I../../clickmodule/LCDMINI/ \
           -O2 -Wall -static -fno-short-enums \
           -Wl,--unresolved-symbols=ignore-in-shared-libs 



AM_LDFLAGS = -static -lhw 

AM_CXXFLAGS = -std=c++14 -std=gnu++11 

clean-generic:
	-$(MAKE) 

bin_PROGRAMS = lcdmini_demo

lcdmini_demo_SOURCES = lcdmini_demo.c
lcdmini_demo_LDADD = libclick.a 

noinst_LIBRARIES = libclick.a 

libclick_a_SOURCES = ../../clickmodule/LCDMINI/lcdmini.c 

