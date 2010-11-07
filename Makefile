# Project: net-- library

BIN         = libnet--.a
SRCFILES    = netpacket.cpp netbase.cpp netclient.cpp netserver.cpp
HEADERS     = netpacket.h netbase.h netclient.h netserver.h
INCLUDES    = 
LOGFILES    = network.log
DEBUG       = on

#Enable this for complete packet dumps.  Consider using Wireshark instead!
####MOREFLAGS   = -DDEBUG_PACKET

#Build rules for a library in MinGW
include makefiles/lib.MinGW.mak

#What to do for make install
INSTALL_INCLUDE = ../include
INSTALL_LIB = ../lib

install:
	cp $(addprefix $(SRCDIR)/, $(HEADERS)) $(INSTALL_INCLUDE)/
	cp $(BIN) $(INSTALL_LIB)/

#How to uninstall
uninstall:
	-rm $(addprefix $(INSTALL_INCLUDE)/, $(HEADERS))
	-rm $(INSTALL_LIB)/$(BIN)
