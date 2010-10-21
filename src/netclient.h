//netClient.h

#ifndef NETCLIENT_H
#define NETCLIENT_H


#include "netbase.h"
class netClient : public netbase {

public:
    netClient();
    ~netClient();

    //Network functions
    int doConnect( const string& address, int remotePort, int localPort = 0);
    int doDisconnect();
    int sendMessage( short i, short len, unsigned char* d, short ver=NET_MSG_VERSION);
    int run();      //Look for incoming messages
    bool setConnTimeout( int seconds=3, int microsec=0);


protected:
    struct timeval connTimeout;     //Connection timeout
    int sdServer;
    short serverPort;

    int closeSocket( int sd);

};

#endif
