# Test of libnet-- netclient class.  Connect to HTTP and sent GET request.

BIN         = test_client.exe
SRCFILES    = test_client.cpp
LIBS        = -L../ -lnet-- -lwsock32
INCLUDES    = -I../src
LOGFILES    = network.log
###DEBUG       = on


#Build rules for a binary in MinGW
include ../makefiles/bin.MinGW.mak


#How to install
INSTALL_BIN = ../
install:
	cp $(BIN) $(INSTALL_BIN)/

#How to uninstall
uninstall:
	-rm $(INSTALL_BIN)/$(BIN)
