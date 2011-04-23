# Test of libnet-- netserver class.  Listen on HTTP port,
#   send a canned HTML response.

BIN         = test_http.exe
SRCFILES    = test_http.cpp
LIBS        = -L/usr/local/lib -lnet-- -lwsock32
INCLUDES    = -I/usr/local/include
LOGFILES    = network.log
###DEBUG       = on

#How to install
INSTALL_BIN = ../

#Build rules for a binary in MinGW
include ../bin.MinGW.mak
