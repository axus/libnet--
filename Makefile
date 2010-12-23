# Project: net-- library

BIN         = libnet--.a
SRCFILES    = netpacket.cpp netbase.cpp netclient.cpp netserver.cpp
HEADERS     = netpacket.h netbase.h netclient.h netserver.h
INCLUDES    = 
LOGFILES    = network.log
###DEBUG       = on

#Enable this for complete packet dumps.  Consider using Wireshark instead!
###MOREFLAGS   = -DDEBUG_PACKET

#What to do for make install
INSTALL_INC = /usr/local/include
INSTALL_LIB = /usr/local/lib

#Build rules for a library in MinGW
include lib.MinGW.mak
