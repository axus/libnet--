// new netserver.h
#ifndef netserver_H
#define netserver_H

//
//      Class for starting a network server: bind a socket, listen on a port
//          handle client connections, etc.
//

#include "netbase.h"

//
// Class definition
//

class netserver : public netbase {

public:
    netserver(unsigned int max);
    ~netserver();
    int openPort(short port);
    void closePort();        //close the server (and server.log)
    int run();         //Check the network: read sockets, handle callbacks

    static size_t cb_incoming( netpacket* pkt, void *cb_data);   //Default function to handle incoming packets

protected:
    fd_set listenSet;   //set of port listening file descriptors
    short serverPort;   //Listening port number
    int sdListen;       //Listening socket number
    
    int closeSocket(int);   //Overloaded function....
    int buildListenSet();   //Rebuild listenSet
    int checkPort();        //Examine port for connections
    int acceptConnection(); //Handle an incoming connection
    size_t handleIncoming( netpacket* pkt);   //Default function to handle incoming packets
    

};

#endif
