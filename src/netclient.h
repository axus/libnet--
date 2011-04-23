//netClient.h
//  See netbase.h for base functions

#ifndef NETCLIENT_H
#define NETCLIENT_H

#include "netbase.h"

namespace net__ {
    class netclient : public netbase {
    
    public:
        netclient( unsigned int maxConnections);
        ~netclient();
    
        //Open a connection and return connection ID
        sock_t doConnect( const std::string& address,
                          uint16_t remotePort, uint16_t localPort = 0);
        int run();      //Look for incoming messages
        bool setConnTimeout( int seconds=3, int microsec=0);
    
    
    protected:
        struct timeval connTimeout;     //Connection timeout
    
    };
}

#endif
