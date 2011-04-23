// new netserver.h
#ifndef netserver_H
#define netserver_H

//
//      Class for starting a network server: bind a socket, listen on a port
//          handle client connections, etc.
//

//
//  Remember that sockets are stream based...
//      the second half of a message may be delayed!
//

#include "netbase.h"

//
// Class definition
//

namespace net__ {
    class netserver : public netbase {
    
    public:
        netserver(unsigned int max);
        ~netserver();
        
        //open *port*, return socket descriptor
        sock_t openPort(int16_t port);
        
        //close the server (and server.log)
        void closePort();
        
        //Check the network: read sockets, handle callbacks
        int run();
    
    protected:
          //Ready to continue?
        bool ready;
          //set of port listening file descriptors
        fd_set listenSet;
          //Listening port number
        int16_t serverPort;
          //Listening socket number
        sock_t sdListen;
        
        //Overloaded function from netbase....
        int closeSocket(sock_t);
        
        //Rebuild listenSet
        int buildListenSet();
        
        //Examine port for connections
        int checkPort();
        
         //Handle an incoming connection, return socket number
        sock_t acceptConnection();
        
    };
}
#endif
