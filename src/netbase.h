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
#include <vector>

//
//  Class definition
//

class netbase {

public:

    //Function pointer types
    typedef size_t (*connectionFP)( int ID, void *cb_data);

    //Constructors
    netbase(unsigned int);
    virtual ~netbase();

    //Send packet "pkt" on socket "sd"
    int sendPacket( int sd, netpacket &pkt);

    //Close socket "sd"
    bool disconnect( int sd);

    //Add a callback for incoming packets on matching connection *c*
    bool setConPktCB( int c, netpacket::netPktCB cbFunc, void *cbData );
    
    //Remove callbacks for connection *c*
    bool unsetConPktCB( int c);

    //Remove generic and connection-specific incoming packet callbacks
    void unsetAllPktCB();

    //Set callback for what to do when new connection is created
    void setConnectCB( connectionFP cbFunc, void *cbData);

    //Set callback for what to do when connection drops on other side
    void setDisconnectCB( connectionFP cbFunc, void *cbData );
    
    //Remove disconnect callback
    void removeDisconnectCB();

    //const functions
    bool isClosed(int sd) const;   //Is socket closed?

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
    bool ready;             //Ready to continue?
    
    fd_set sdSet;   //set of all file descriptors
    std::set<int> conSet;   //Currently connected sockets
    std::set<int> closedSocketSet;   //Sockets which are pending disconnection
    std::set<int> pendingSet;   //unprocessed data is pending on these sockets
        
    int sdMax;              //Max socket descriptor in conSet
    unsigned int conMax;    //Max connections allowed
    
    //Use a buffer for all incoming packets
    //uint8_t *myBuffer;
    
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
    std::map< int, netpacket::netPktCB > packetCB_map;
    std::map< int, void* > packetCBD_map;

    //Map socket descriptor to sequential index, for memory pointing fun.
    std::map<int, size_t> conIndexMap;

    //Modify a socket to be non-blocking
    int unblockSocket(int); 
    
    //Create the set of sockets
    int buildSocketSet();
    
    //Select on socketSet until incoming packets complete
    int readIncomingSockets();
    
    //Read set of sockets, return list of netpackets
    std::vector<netpacket*> readSockets();
    
    //Fire callbacks for list of packets (and disconnected sockets)
    int fireCallbacks(std::vector<netpacket*>& packets);
    
    //Receive data on a socket to a buffer
    int recvSocket(int sd, uint8_t* buffer);
    
    //Closes socket, sets it to INVALID_SOCKET
    virtual int closeSocket(int sd);
    
    //Erase socket descriptor from conSet
    int removeSocket(int sd);
    
    //Free buffer associated with connection
    void cleanSocket(int sd);

    //Create netpacket from buffer
    //We must delete it when finished!!
    netpacket* makePacket( int ID, uint8_t* buffer, size_t write_pos);
    
    //Debugging helpers
    void debugBuffer( uint8_t* buffer, size_t buflen) const;
    std::string getSocketError() const;

    //Default incoming packet callback.  Return size of packet.
    static size_t incomingCB( netpacket* pkt, void *CBD);
    
    //Default connect/disconnect callbacks.  Return socket descriptor.
    static size_t connectionCB( int con, void *CBD);
    static size_t disconnectionCB( int con, void *CBD);
};

#endif
