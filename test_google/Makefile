# Test: Connect to Google, send HTTP GET request, print incoming packets

BIN         = test_google.exe
SRCFILES    = test_google.cpp
LIBS        = -L../ -lnet-- -lwsock32
INCLUDES    = -I../src -DDEBUG

#Build rules for a library in MinGW
include src/bin.MinGW.mak

#What to do for make install
INSTALL_BIN = ../

clean2: clean
	rm network.log
	
install:
	cp $(BIN) $(INSTALL_BIN)/

#How to uninstall
uninstall:
	-rm $(INSTALL_BIN)/$(BIN)
