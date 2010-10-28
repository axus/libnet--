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

    //Function pointer types
    typedef size_t (*connectionFP)( int ID, void *cb_data);

    //Constructors
    netbase(unsigned int);
    virtual ~netbase();

    //Send packet "pkt" on socket "sd"
    int sendPacket( int sd, netpacket &pkt);

    //Set incoming packet callback.  Return value of cbFunc *MUST* be number of bytes consumed.
    void setPktCB( netpacket::netPktCB cbFunc, void *cbData);

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
    bool isConnected() const;
    bool isClosed(int sd) const;   //Is socket closed?

    //Logging functions
    bool openLog() const;     //will open the debugLog, if not open already
    bool closeLog() const;    //close the debugLog if open

    //Public data members
    mutable std::ofstream debugLog;  //want this to be publicly accessible
    mutable std::string lastError;  //External classes should check this for error string
    static const size_t NETMM_MAX_RECV_SIZE   = 0x40000;   //256K

protected:
    //Constants
    static const size_t NETMM_MEMORY_SIZE     = 0x1000000; //16MB

    //Network parameters
    struct timeval timeout; //Timeout interval
    bool ready;             //Ready to continue?
    
    fd_set sdSet;   //set of all file descriptors
    std::set<int> conSet;   //set of socket connections which are current connected
    int sdMax;              //Max socket descriptor in conSet
    unsigned int conMax;    //Max connections allowed
    
    //Use a buffer for all incoming packets
    unsigned char *myBuffer;
    //TODO: buffer per connection, that 
    
    size_t myIndex; //offset in buffer
    size_t lastMessage;  //Increment each time a message is sent out

    //Function pointer for when a new connection is received
    connectionFP conCB;
    void *conCBD;

    //Function pointer for when disconnection occurs
    connectionFP disCB;
    void *disCBD;

    //Default function and data for incoming packets
    netpacket::netPktCB allCB; 
    void *allCBD;

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
    
    //Read set of socket 
    int readSockets();
    
    //Receive data on a socket to a buffer
    int recvSocket(int sd, unsigned char* buffer);
    
    //Closes socket, sets it to INVALID_SOCKET
    virtual int closeSocket(int sd);      

    //Create netpacket from data on incoming connection
    //We must delete it when finished!!
    netpacket* getPacket( int ID, unsigned char* buffer, short len);
    
    //Debugging helpers
    void debugBuffer( unsigned char* buffer, int buflen) const;
    std::string getSocketError() const;

    //Default incoming packet callback.  Return size of packet.
    static size_t incomingCB( netpacket* pkt, void *CBD);
    
    //Default connect/disconnect callbacks.  Return socket descriptor.
    static size_t connectionCB( int con, void *CBD);
    static size_t disconnectionCB( int con, void *CBD);
};

#endif
