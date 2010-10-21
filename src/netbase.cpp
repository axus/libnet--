// netbase: Create connections, send and receive data through them
//      Assign callbacks for "netpacket"s that return true for specified function

#include "netbase.h"
#include <iostream>
#include <iomanip>
#include <vector>

using std::set;
using std::map;
using std::pair;
using std::ofstream;
using std::string;
using std::vector;

using std::cerr;
using std::endl;
using std::hex;
using std::dec;
using std::ios;
using std::setfill;
using std::setw;

//Constructor, specify the maximum client connections
netbase::netbase(unsigned int max): ready(false), sdMax(-1), conMax( max),
    myBuffer(NULL), myIndex(0), lastMessage(-1), allCB(doNothing), allCBD(NULL)
{
    //Allocate memory for the ring buffer where incoming packets will drop
    myBuffer = new unsigned char[NET_MEMORY_SIZE << 1];

    //Zero out the socket descriptor set
    FD_ZERO( &sdSet);

    //Start debug log
    openLog();

    //First timeout is 500ms
    timeout.tv_sec = 0;
    timeout.tv_usec = 500;

#ifdef _WIN32
	WSADATA wsaData;
	int wsaret = WSAStartup(0x0202, &wsaData);
	if(wsaret!=0) {
        debugLog << "Error starting WSA" << endl;
        return;
    }
    if (wsaData.wVersion != 0x0202)
    { // wrong WinSock version!
        debugLog << "Current winsock version " << hex << wsaData.wVersion << " unsupported" << endl << dec;
        WSACleanup (); // unload ws2_32.dll
        return;
    }
#endif
}

//Destructor
netbase::~netbase()
{
    //TODO: disconnect everything still in the conSet
    WSACleanup();
}


//Associate a callback function to all packets received
//long netbase::addCB( size_t (*functionCB)( netpacket&, void*), void* CBD )
bool netbase::addCB( netpacket::netMsgCB cbFunc, void* dataCBD  )
{
    allCB = cbFunc;
    allCBD = dataCBD;
    
    return true;
}

//Add callback for specific conditions
bool netbase::addCB( testPacketFP& testFunc, netpacket::netMsgCB& cbFunc, void* dataCBD )
{
    CB_functions.insert( pair< testPacketFP, netpacket::netMsgCB/* short (*)( netpacket&, void*)*/ >( testFunc, cbFunc ));
    CB_datas.insert( pair< testPacketFP, void* >( testFunc, dataCBD ));

    //TODO: support multiple callbacks on the same mapkey
    return true;
}


//Remove callback associated with ID/version pair
bool netbase::removeCB( testPacketFP& testFunc)
{
    bool result=true;

    //Erase everything matching the key
    if (CB_functions.erase( testFunc ) ==0) {
        debugLog << "No callback function." << endl;
        result = false;
    }
    if (CB_datas.erase( testFunc) == 0) {
        debugLog << "No callback data." << endl;
    }
    //TODO: remove multiple callbacks

    return result;
}


//Clear the callback data structures
void netbase::removeAllCB()
{
    CB_functions.clear();
    CB_datas.clear();
    allCB = doNothing;
    allCBD = NULL;

    return;
}


//Find the callback function and data matching the ID and version
bool netbase::getCB(testPacketFP& testFunc, netpacket::netMsgCB& functionCB, void*& dataCB) const
{
    map< testPacketFP, netpacket::netMsgCB >::const_iterator iter;
    map< testPacketFP, void*>::const_iterator iter_data;
    
    //Find the related callback function
    debugLog << hex;
    iter = CB_functions.find( testFunc );
    if (iter != CB_functions.end()) {
        functionCB = iter->second;  //Assign value to parameter here
        debugLog << "Callback: CB=0x" << (int)functionCB << endl;
    }
    else {
        functionCB = NULL;
        return false;
    }
    debugLog << dec;

    //Find the related callback data
    iter_data = CB_datas.find( testFunc);
    if (iter_data != CB_datas.end())
        dataCB = iter_data->second; //Assign value to next parameter here
    else
        dataCB = NULL;

    return true;
}


//check connection status
bool netbase::isConnected() const {
    return ready;
}


//Send message on a connection 'c'
int netbase::sendMessage( int connection, netpacket &msg) {

    int rv;
    const short length = msg.get_length();

    if (!ready) {
        debugLog << "Error: Disconnected, can't send" << endl;
        return -1;
    }

    //Check if connection number exists in conSet
    if ( conSet.find( connection ) == conSet.end() ) {
        debugLog << "Error: No connection " << connection << " to send on" << endl;
        return -1;
    }

    //Trying to send on socket
    rv = send(connection, (const char*)msg.get_ptr(), length, /*flags*/0);

    //TODO: while (rv > 0 && totalSent < length + NET_MSG_HEADER_BYTES
    if (rv == SOCKET_ERROR) {
        debugLog << "Socket Error:" << getSocketError() << endl;
        return -1;
    }
    
    //Record the message information, how much was sent
    debugLog    << "Sent, rv=" << rv << "/" << length << " bytes" << endl;
                
    if (rv == 0) {
        debugLog << "Warning: Message not sent" << endl;
        return 0;
    }
    if (rv < length ) {
        //TODO: send the rest of the message
        debugLog << "Warning: Send incomplete" << endl;
    }
    
    return rv;
}


//Setup a socket to be non-blocking and reusable
int netbase::unblockSocket(int sd) {

    if (sd == (int) INVALID_SOCKET) {
        debugLog << "Can't unblock an invalid socket" << endl;
        return -1;
    }

    // set fd to be non-blocking
#ifdef _WIN32    //Windows
    u_long flags = 1;
    if (ioctlsocket(sd, FIONBIO, &flags) == -1) {
        debugLog << "FIONBIO error: " << GetLastError() << endl;
        return -1;
    }
#else           //Unix
    int flags = fcntl(sd, F_GETFL, 0);
    flags |= O_NONBLOCK;
#ifdef O_NDELAY
    flags |= O_NDELAY;
#endif

    if (fcntl(sd, F_SETFL, flags) == -1) {
        debugLog << "Error setting non-blocking socket" << endl;
        closeSocket(sd);
        return -1;
    }

#endif

    //Set socket lingering options (Don't linger... background will handle it).
    flags = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_DONTLINGER,
           (char *)&flags, sizeof(flags)) < 0) {
        debugLog  << "Error for socket lingering" << endl;
        closeSocket(sd);
        return -1;
    }
    
    // set REUSEADDR / REUSEPORT flags
    flags = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,
           (char*) &flags, sizeof(flags)) < 0) {
        debugLog  << "Error setting resusable address" << endl;
        closeSocket(sd);
        return -1;
    }

#ifdef SO_REUSEPORT
    flags = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEPORT,
        (char*) &flags, sizeof(flags)) < 0) {
        debugLog << "Error setting reusable port" << endl;
        closeSocket(sd);
        return -1;
    }
#endif
    return 0;
}

//This creates an FD_SET from the server socket and all current client sockets
//This must be called before each new "select()" call
int netbase::buildSocketSet()
{
    set<int>::const_iterator iter;

    FD_ZERO( &sdSet );
    for (iter = conSet.begin(); iter != conSet.end(); iter++)
        FD_SET( (unsigned int)(*iter), &sdSet);

    return 0;

}

// Close a socket, remove it from the list of connections
int netbase::closeSocket(int sd)
{
    int rv;

    openLog();
    if (sd == (int)INVALID_SOCKET) {
        debugLog << "Socket is already closed" << endl;
        return sd;
    }

    rv = closesocket(sd);
    if (rv == SOCKET_ERROR)
        debugLog << "Error closing socket " << sd << ":" << getSocketError() << endl;

    //Remove this socket from the list of connected sockets
    conSet.erase(sd);
    if (FD_ISSET((unsigned int)sd, &sdSet))
        FD_CLR((unsigned int)sd, &sdSet);
        
    return rv;
}


//Check all sockets in sdSet for incoming data, then process callbacks
int netbase::readSockets()
{
    int rv=0, con=0;
    set<int>::const_iterator con_iter;
    vector< netpacket * > packets;

    //Check all connections in conSet
    for (con_iter = conSet.begin(); con_iter != conSet.end(); con_iter++) {
        con = *con_iter;
        if (FD_ISSET( con, &sdSet)) {
            //Data pending from client, get the message (or disconnect)
            size_t startIndex = myIndex;
            rv = recvSocket( con, (myBuffer + startIndex) );
            if ( rv > 0 ) {
            
                //Read from buffer into new packet object
                netpacket *pkt = getPacket( con, (myBuffer + startIndex), rv);
                packets.push_back( pkt);
            }
            else {
                //Call associated disconnect callback right away
                map< int, disconnectFP >::const_iterator dcb_iter;
                dcb_iter = disconnectCB_map.find(con);
                if ( dcb_iter != disconnectCB_map.end() ) {
                    //Call the disconnection callback, with mapped void *data
                    disconnectFP dcFunc = (*dcb_iter).second;
                    dcFunc( con, disconnectCBD_map.find(con)->second);
                }
                else {
                    debugLog << "Client quit, no callback for disconnect" << endl;
                }
            }
        }
    }
    
    //All pending data has been read, fire callbacks now
    vector< netpacket * >::const_iterator pkt_iter;
    map< testPacketFP, netpacket::netMsgCB >::const_iterator cb_iter;
    
    for (pkt_iter = packets.begin(); pkt_iter != packets.end(); pkt_iter++) {

        //Packet pointer
        netpacket *pkt = *pkt_iter;
        
        //Perform the "main" callback
        allCB( pkt, allCBD);
        
        //Try special case callbacks
        for (cb_iter = CB_functions.begin(); cb_iter != CB_functions.end(); cb_iter++) {
            testPacketFP testFunc = cb_iter->first;
            void *cb_data = CB_datas.find(testFunc)->second;
            
            //Check if packet matches condition
            if (testFunc( pkt->get_ptr(), cb_data)) {
              
                //Get callback function and callback data
                netpacket::netMsgCB pktcb = cb_iter->second;
                
                //Finally, run the callback! Could happen for multiple conditions
                pktcb( pkt, cb_data);
            }
        }
    }
    
    //Delete the dynamically created packet objects
    for (pkt_iter = packets.begin(); pkt_iter != packets.end(); pkt_iter++) {
        delete *pkt_iter;
    }
    
    //If myIndex exceeds threshold, set it back to 0.
    //We'd better be done with all the packets!!
    if (myIndex > NET_MEMORY_SIZE) {
        myIndex = 0;
    }
    
    return rv;
}


//Recieve incoming data on a buffer, return the number of bytes read in
int netbase::recvSocket(int sd, unsigned char* buffer)
{
    int rv;

    do {
        //Read incoming bytes to buffer
        rv = recv( sd, (char*)buffer, NET_MAX_RECV_SIZE,  0 );
    
        if (rv == 0) {
            debugLog << "Socket " << sd << " disconnected" << endl;
            closeSocket(sd);
            return 0;
        }
        
        //Check for reset socket, which we treat as normal disconnect since we are lazy
        #ifdef _WIN32
        if (WSAGetLastError() == WSAECONNRESET) {
            debugLog << "Socket " << sd << " quit" << endl;
            closeSocket(sd);
            return 0;
        }
        #endif
        
        if (rv == SOCKET_ERROR) {
            debugLog << "Receive error on " << sd << ": " << getSocketError() << endl;
            closeSocket(sd);
            return -1;
        }
    
        //rv was not 0 or SOCKET_ERROR, so it's the number of bytes received.
        myIndex += rv;

    } while (rv == (int)NET_MAX_RECV_SIZE);  //Keep reading if recv buffer is full
    
    debugLog << "Received " << rv << " bytes on socket " << sd << endl;

    return rv;
}


//Output the contents of a buffer to the log
void netbase::debugBuffer( unsigned char* buffer, int buflen) const
{
    int byte, width=16;
    unsigned short curr_word;

    debugLog << "::: Buffer contents ::: " << endl << hex;

    //Dump the buffer contents
    for (byte = 0; byte < buflen - 1; byte += 2) {

        memcpy( &curr_word, &(buffer[byte]), 2);
        debugLog << "0x" << setfill('0') << setw(4) << curr_word;

        if ((byte % width ) == (width - 2))
            debugLog << endl;
        else
            debugLog << " ";
    }

    //If it is off a word boundary, dump the last byte
    if (byte == buflen - 1)
        debugLog << " 0x" << hex << setw(2) << (int)buffer[byte] << dec;

    if ( (byte % width) > 0)
        debugLog << endl;
}

//New packet object, pointing at the data in our big ring buffer
netpacket *netbase::getPacket( int con, unsigned char *buffer, short len)
{
    netpacket *result = new netpacket( len, buffer );
    result->ID = con;
    
    return result;
}

//Get the most recent socket error from the system
string netbase::getSocketError() const
{
    string result;

    result = "Unknown network error";
#ifdef _WIN32
    switch ( WSAGetLastError()) {
        case WSANOTINITIALISED:
            result = "Server did not initialize WSA";
            break;
        case WSAENETDOWN:
            result = "Network failure";
            break;
        case WSAEADDRINUSE:
            result = "Address in use";
            break;
        case WSAEINTR:
            result = "Socket access interrupted";
            break;
        case WSAEINPROGRESS:
            result = "A socket is blocking";
            break;
        case WSAEADDRNOTAVAIL:
            result = "Address not available";
            break;
        case WSAEAFNOSUPPORT:
            result = "Address not supported";
            break;
        case WSAECONNREFUSED:
            result = "Connection (forcefully) refused";
            break;
        case WSAEDESTADDRREQ :
            result = "Address required";
            break;
        case WSAENOTCONN:
            result = "Socket already disconnected";
            break;
        case WSAEFAULT:
            result = "Programming mistake";
            break;
        case WSAEINVAL:
            result = "Unbound or invalid socket";
            break;
        case WSAEISCONN:
            result = "Already connected";
            break;
        case WSAEMFILE:
            result = "No more descriptors left";
            break;
        case WSAENETUNREACH:
            result = "Destination unreachable";
            break;
        case WSAENOBUFS:
            result = "Out of buffer space";
            break;
        case WSAENOTSOCK:
            result = "Non-socket descriptor";
            break;
        case WSAETIMEDOUT:
            result = "Connection timeout";
            break;
        case WSAEWOULDBLOCK:
            result = "Blocking on non-blocking socket";
            break;
        case WSAEOPNOTSUPP:
            result = "Unsupported socket operation";
            break;
        case WSAESHUTDOWN:
            result = "Socket already shutdown";
            break;
        case WSAEMSGSIZE:
            result = "Recieved oversized message";
            break;
        case WSAECONNABORTED:
            result = "Socket was lost";
            break;
        case WSAECONNRESET:
            result = "Socket was reset";
            break;
        default:
            result = "Unknown winsock error";
            break;
    }
#endif

    return result;
}

//Start the debugging log
bool netbase::openLog() const
{
#ifdef DEBUG
    if ( ! debugLog.is_open() )
        debugLog.open("network.log", ios::out | ios::app);
#endif
    return (debugLog.is_open());
}


//Stop the debugging log
bool netbase::closeLog() const
{
#ifdef DEBUG
    if ( debugLog.is_open() )
        debugLog.close();
#endif
    return true;
}
