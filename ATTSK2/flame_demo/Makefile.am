AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
ACLOCAL_ARFLAGS = cr

AM_CPPFLAGS = -I. -I../../clickmodules/FLAME/ \
           -O2 -Wall -static -fno-short-enums \
           -Wl,--unresolved-symbols=ignore-in-shared-libs 

AM_LDFLAGS = -static -lhw 

AM_CXXFLAGS = -std=c++14 -std=gnu++11 

clean-generic:
	-$(MAKE) 

bin_PROGRAMS = flame_demo 

flame_demo_source = flame_demo.c 

noinst_LIBRARIES   = libclick.a 
libclick_a_SOURCES = ../../clickmodules/FLAME/flame.c 

flame_demo_LDADD = libclick.a 


