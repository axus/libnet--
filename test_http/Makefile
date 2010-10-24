# Test of libnet-- netserver class.  Listen on HTTP port, send canned HTML response.

BIN         = test_http.exe
SRCFILES    = test_http.cpp
LIBS        = -L../ -lnet-- -lwsock32
INCLUDES    = -I../src
LOGFILES    = network.log
DEBUG       = on


#Build rules for a binary in MinGW
include ../makefiles/bin.MinGW.mak


#How to install
INSTALL_BIN = ../
install:
	cp $(BIN) $(INSTALL_BIN)/

#How to uninstall
uninstall:
	-rm $(INSTALL_BIN)/$(BIN)
