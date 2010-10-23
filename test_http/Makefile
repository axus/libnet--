# Test: Connect to Google, send HTTP GET request, print incoming packets

BIN         = test_http.exe
SRCFILES    = test_http.cpp
LIBS        = -L../ -lnet-- -lwsock32
INCLUDES    = -I../src
DEBUG       = on

#Build rules for a library in MinGW
include src/bin.MinGW.mak

#What to do for make install
INSTALL_BIN = ../

clean2: clean
	-rm -f network.log
	
install:
	cp $(BIN) $(INSTALL_BIN)/

#How to uninstall
uninstall:
	-rm $(INSTALL_BIN)/$(BIN)
