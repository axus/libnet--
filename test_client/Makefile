# Test: Connect to Google, send HTTP GET request, print incoming packets

BIN         = test_client.exe
SRCFILES    = test_client.cpp
LIBS        = -L../ -lnet-- -lwsock32
INCLUDES    = -I../src
DEBUG       = on

INSTALL_BIN = ../

#Build rules for a binary in MinGW
include src/bin.MinGW.mak

#Different cleaning options
clean2: clean
	-rm -f network.log
	
#How to install
install:
	cp $(BIN) $(INSTALL_BIN)/

#How to uninstall
uninstall:
	-rm $(INSTALL_BIN)/$(BIN)
