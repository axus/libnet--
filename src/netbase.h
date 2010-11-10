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

#ifndef _MSC_VER
    #include <sys/time.h>
#endif

#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

//
//  Class definition
//

class netbase {

public:

    //Function pointer types
    typedef size_t (*connectionFP)( sock_t sd, void *cb_data);

    //Constructors
    netbase(size_t);    //Maximum connections
    virtual ~netbase();

    //Send packet "pkt" on socket "sd"
    size_t sendPacket( sock_t sd, netpacket &pkt);

    //Close socket "sd", and remove connection specific callbacks
    bool disconnect( sock_t sd);

    //Add a callback for incoming packets on matching connection *c*
    bool setConPktCB( sock_t sd, netpacket::netPktCB cbFunc, void *cbData );
    
    //Remove callbacks for connection *c*
    bool unsetConPktCB( sock_t c);

    //Remove generic and connection-specific incoming packet callbacks
    void unsetAllPktCB();

    //Set callback for what to do when new connection is created
    void setConnectCB( connectionFP cbFunc, void *cbData);

    //Set callback for what to do when connection drops on other side
    void setDisconnectCB( connectionFP cbFunc, void *cbData );
    
    //Remove disconnect callback
    void removeDisconnectCB();

    //const functions
    bool isClosed(sock_t sd) const;   //Is socket closed?

    //Logging functions
    bool openLog() const;     //will open the debugLog, if not open already
    bool closeLog() const;    //close the debugLog if open
    size_t debugPacket(const netpacket *pkt) const;

    //Public data members
    mutable std::ofstream debugLog;  //want this to be publicly accessible
    mutable std::string lastError;  //External classes should check this for error string
    static const size_t NETMM_MAX_RECV_SIZE   = 0x10000;   //64K
    static const size_t NETMM_MAX_SOCKET_DESCRIPTOR = 0xFFFF; //64K
    static const size_t NETMM_CON_BUFFER_SIZE = (NETMM_MAX_RECV_SIZE << 1);   //128K

protected:
    //Constants
    static const size_t NETMM_MEMORY_SIZE     = 0x1000000; //16MB

    //Network parameters
    struct timeval timeout; //Timeout interval
    
    fd_set sdSet;   //set of all file descriptors
    std::set<sock_t> conSet;   //Currently connected sockets
    std::set<sock_t> closedSocketSet;   //Sockets which are pending disconnection
        
    sock_t sdMax;           //Max socket descriptor in conSet
    size_t conMax;    //Max connections allowed
    
    //Each connection gets its own buffer
    uint8_t* conBuffer[NETMM_MAX_SOCKET_DESCRIPTOR];
    size_t conBufferIndex[NETMM_MAX_SOCKET_DESCRIPTOR];
    size_t conBufferLength[NETMM_MAX_SOCKET_DESCRIPTOR];
    size_t conBufferSize[NETMM_MAX_SOCKET_DESCRIPTOR];
    
    // Buffer         Index   Length                                       Size
    // |  (consumed)    |       |                                             |
    // |----------------------------------------------------------------------|
    
    size_t lastMessage;  //Increment each time a message is sent out

    //Function pointer for when a new connection is received
    connectionFP conCB;
    void *conCBD;

    //Function pointer for when disconnection occurs
    connectionFP disCB;
    void *disCBD;

    //Map connection IDs to callback function/data for incoming packets
    std::map< sock_t, netpacket::netPktCB > packetCB_map;
    std::map< sock_t, void* > packetCBD_map;

    //Map socket descriptor to sequential index, for memory pointing fun.
    std::map<sock_t, size_t> conIndexMap;

    //Modify a socket to be non-blocking
    int unblockSocket(sock_t sd); 
    
    //Create the set of sockets
    size_t buildSocketSet();
    
    //Select on socketSet until incoming packets complete
    int readIncomingSockets();
    
    //Read set of sockets, return list of netpackets
    std::vector<netpacket*> readSockets();
    
    //Fire callbacks for list of packets (and disconnected sockets)
    int fireCallbacks(std::vector<netpacket*>& packets);
    
    //Receive data on a socket to a buffer
    int recvSocket(sock_t sd, uint8_t* buffer);
    
    //Closes socket, sets it to INVALID_SOCKET
    virtual int closeSocket(sock_t sd);
    
    //Erase socket descriptor from conSet
    int removeSocket(sock_t sd);
    
    //Free buffer associated with connection
    void cleanSocket(sock_t sd);

    //Create netpacket from buffer.  **delete it when finished**
    netpacket* makePacket( sock_t ID, uint8_t* buffer, size_t pkt_size);
    
    //Debugging helpers
    void debugBuffer( uint8_t* buffer, size_t buflen) const;
    std::string getSocketError() const;

    //Default incoming packet callback.  Return size of packet.
    static size_t incomingCB( netpacket* pkt, void *CBD);
    
    //Default connect/disconnect callbacks.  Return socket descriptor.
    static size_t connectionCB( sock_t con, void *CBD);
    static size_t disconnectionCB( sock_t con, void *CBD);
};

#endif
