# Test of libnet-- netserver class.  Listen on HTTP port, send canned HTML response.

BIN         = test_http.exe
SRCFILES    = test_http.cpp
LIBS        = -L../build -lnet-- -lwsock32
INCLUDES    = -I../src
LOGFILES    = network.log
###DEBUG       = on

#How to install
INSTALL_BIN = ../

#Build rules for a binary in MinGW
include ../bin.MinGW.mak
