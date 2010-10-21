//netbase.h
#ifndef netbase_H
#define netbase_H

//
//      Base class for network clients/servers using <winsock.h> and "netpacket.h"
//


#include "netpacket.h"

#include <winsock2.h>
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

    //type netbase::testPacketFP is a function pointer for functions that test packet array for matching condition
    typedef bool (*testPacketFP)( const unsigned char* pkt_data, void *cb_data);
    typedef size_t (*disconnectFP)( int ID, void *cb_data);

    //Constructors
    netbase(unsigned int);
    virtual ~netbase();

    //Add a callback for any packet
    bool addCB( netpacket::netMsgCB cbFunc, void* dataCBD );

    //Add a callback for packets where testFunc() is true. Callback the cbFunc function.
    bool addCB( testPacketFP &testFunc, netpacket::netMsgCB& cbFunc, void* dataCBD );
    
    //Return the callback triggered where testFunc() is true.
    bool getCB( testPacketFP &testFunc, netpacket::netMsgCB& cbFunc, void*& dataCBD ) const;
    
    //Remove callbacks triggered on testFunc
    bool removeCB( testPacketFP& testFunc );
    
    //Remove ALL callbacks
    void removeAllCB();

    //const functions
    bool isConnected() const;

    //Send message "msg" on connection "c"
    int sendMessage( int c, netpacket &msg);

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

    //Callback function and data for any packet    
    netpacket::netMsgCB allCB; 
    void *allCBD;
    
    //Map connection IDs to callback function/data for dropped connections
    std::map< int, disconnectFP > disconnectCB_map;
    std::map< int, void* > disconnectCBD_map;
    

    //map packet tester function pointer -> callback function
    std::map< testPacketFP, netpacket::netMsgCB > CB_functions;    
    /*short (*)( netpacket&, void*)*/
    //map packet tester function pointer -> callback data
    std::map< testPacketFP, void* > CB_datas;    

    //Modify a socket to be non-blocking
    int unblockSocket(int); 
    
    //Create the set of sockets
    int buildSocketSet();
    
    //Read set of socket 
    int readSockets();
    
    //Receive data on a socket to a buffer
    int recvSocket(int sd, unsigned char* buffer);
    
    //Closes socket, sets it to INVALID_SOCKET
    virtual int closeSocket(int);      

    //Create netpacket from data on incoming connection
    netpacket* getPacket( int ID, unsigned char* buffer, short len);
    
    //Debugging helpers
    void debugBuffer( unsigned char* buffer, int buflen) const;
    std::string getSocketError() const;

    //Default "allCB" function
    static size_t doNothing( netpacket* pkt, void *CBD) {return 0;};
};


#endif
