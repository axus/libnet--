//netbase.h
#ifndef netbase_H
#define netbase_H

//
//      Base class for network clients/servers using <winsock.h> and "netpacket.h"
//


#include "netpacket.h"

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/select.h>
#endif

#include <sys/time.h>
#include <set>
#include <map>
#include <iostream>
#include <fstream>

//
//  Class definition
//

class netbase {

public:

    //Constants
    static const size_t NET_MSG_VERSION     = 0x0001;
    static const size_t NET_WSA_VER         = 0x0202;
    static const size_t NET_MAX_ID          = 0xFFFF;
    static const size_t NET_MAX_VERSION     = 0xFFFF;
    static const size_t NET_CB_VERSION      = 0x4000;
    static const size_t NET_CONNECT_ID      = 0x0000;
    static const size_t NET_DISCONNECT_ID   = 0x0001;
    static const size_t NET_MAX_RECV_SIZE   = 0x40000;   //256K
    static const size_t NET_MEMORY_SIZE     = 0x1000000; //16MB

    typedef size_t (*disconnectFP)( int ID, void *cb_data);
    typedef size_t (*newConnFP)( int ID, void *cb_data);

    //Constructors
    netbase(unsigned int);
    virtual ~netbase();

    //Add a callback for incoming packets on matching connection *c*
    bool addCB( int c, netpacket::netPktCB cbFunc, void *cbData );
    
    //Remove callbacks for connection *c*
    bool removeCB( int c);

    //Remove callbacks for every connection
    void removeAllCB();

    //Update allCB and allCBD: callback for all incoming packets
    void setPktCB( netpacket::netPktCB cbFunc, void *cbData);

    //Set callback for what to do when new connection is created
    void setConnectCB( newConnFP cbFunc, void *cbData);

    //const functions
    bool isConnected() const;

    //Send packet "pkt" on connection "c"
    int sendPacket( int c, netpacket &pkt);

    //Logging functions
    bool openLog() const;     //will open the debugLog, if not open already
    bool closeLog() const;    //close the debugLog if open

    //Public data members
    mutable std::ofstream debugLog;  //want this to be publicly accessible
    mutable std::string lastError;

protected:
    //Network parameters
    struct timeval timeout; //Timeout interval
    bool ready;             //Ready to continue?
    
    fd_set sdSet;   //set of all file descriptors
    std::set<int> conSet;   //set of socket connections which are current connected
    int sdMax;              //Max socket descriptor in conSet
    unsigned int conMax;    //Max connections allowed
    
    //Use a buffer for all incoming packets
    unsigned char *myBuffer;
    size_t myIndex; //offset in buffer
    size_t lastMessage;  //Increment each time a message is sent out

    //Function pointer for when a new connection is received
    newConnFP conCB;
    void *conCBD;

    //Default function and data for incoming packets
    netpacket::netPktCB allCB; 
    void *allCBD;
    
    //Map connection IDs to callback function/data for dropped connections
    std::map< int, disconnectFP > disconnectCB_map;
    std::map< int, void* > disconnectCBD_map;

    //Map connection IDs to callback function/data for incoming packets
    std::map< int, netpacket::netPktCB > packetCB_map;
    std::map< int, void* > packetCBD_map;

    //Modify a socket to be non-blocking
    int unblockSocket(int); 
    
    //Create the set of sockets
    int buildSocketSet();
    
    //Select on socketSet until incoming packets complete
    int readIncomingSockets();
    
    //Read set of socket 
    int readSockets();
    
    //Receive data on a socket to a buffer
    int recvSocket(int sd, unsigned char* buffer);
    
    //Closes socket, sets it to INVALID_SOCKET
    virtual int closeSocket(int sd);      

    //Create netpacket from data on incoming connection
    netpacket* getPacket( int ID, unsigned char* buffer, short len);
    
    //Debugging helpers
    void debugBuffer( unsigned char* buffer, int buflen) const;
    std::string getSocketError() const;

    //Default functions for function pointers
    static size_t incomingCB( netpacket* pkt, void *CBD);
    static size_t connectionCB( int con, void *CBD);
};


#endif
