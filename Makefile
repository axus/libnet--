# Project: net++ library

BIN = libnet++.a
SRCFILES = netbase.cpp netpacket.cpp netserver.cpp netclient.cpp

#Build rules for a library in MinGW
include src/lib.MinGW.mak

#What to do for make install
install:
	cp src/net++.h .
