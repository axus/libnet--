# Test of libnet-- netclient class.  Connect to HTTP and sent GET request.

BIN         = test_client.exe
SRCFILES    = test_client.cpp
LIBS        = -L../build -lnet-- -lwsock32
INCLUDES    = -I../src
LOGFILES    = network.log
###DEBUG       = on

#How to install
INSTALL_BIN = ../

#Build rules for a binary in MinGW
include ../bin.MinGW.mak
