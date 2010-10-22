//netClient.h
//  See netbase.h for base functions

#ifndef NETCLIENT_H
#define NETCLIENT_H

#include "netbase.h"
class netclient : public netbase {

public:
    netclient();
    ~netclient();

    //Network functions
    int doConnect( const std::string& address, int remotePort, int localPort = 0);
    int doDisconnect();
    int run();      //Look for incoming messages
    bool setConnTimeout( int seconds=3, int microsec=0);


protected:
    struct timeval connTimeout;     //Connection timeout
    int sdServer;
    short serverPort;

    int closeSocket( int sd);

};

#endif
