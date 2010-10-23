// netbase: Create connections, send and receive data through them
//      Assign callbacks for "netpacket"s that return true for specified function

#include "netbase.h"
#include <iostream>
#include <iomanip>
#include <vector>

//using std::set;
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
    myBuffer(NULL), myIndex(0), lastMessage(-1), 
    conCB(connectionCB), conCBD(NULL), allCB(incomingCB), allCBD(NULL)
{
    //Allocate memory for the ring buffer where incoming packets will drop
    myBuffer = new unsigned char[NET_MEMORY_SIZE << 1];

    //Zero out the socket descriptor set
    FD_ZERO( &sdSet);

    //Start debug log
    if (!openLog()) {
#ifdef DEBUG
        cerr << "Could not open debug log!" << endl;
#endif
        ;
    }

    //Don't use timeout for select (but don't block, either)
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

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
    
#ifdef _WIN32
    WSACleanup();
#endif

    //Free space... would get deleted anyways
    delete myBuffer;
}


//Set default callback function and data
void netbase::setPktCB( netpacket::netPktCB cbFunc, void* dataCBD  )
{
    //Assign defaults
    allCB = cbFunc;
    allCBD = dataCBD;
    
}


//Add a callback for incoming packets on matching connection *c*
bool netbase::addCB( int c, netpacket::netPktCB cbFunc, void* cbData )
{
    bool result = true;

    //Map c -> cbFunc
    result = packetCB_map.insert( std::map< int, netpacket::netPktCB >::value_type( c, cbFunc)).second;
    
    //Map c -> cbData
    result = result && packetCBD_map.insert( std::map< int, void* >::value_type( c, cbData)).second;
    
    //Return value: was callback and callback data inserted successfully?
    return result;
}

//Remove callbacks for connection *c*
bool netbase::removeCB( int c)
{
    bool result = true;
    
    //Unmap c -> callback function
    result = (packetCB_map.erase( c) > 0);
    
    //Unmap c -> callback data
    result = result && (packetCBD_map.erase( c) > 0);
    
    //Return value: was there a callback to remove?
    return result;
}


//Clear the incoming packet callbacks
void netbase::removeAllCB()
{
    //CB_functions.clear();
    //CB_datas.clear();
    packetCB_map.clear();
    packetCBD_map.clear();
    allCB = incomingCB;
    allCBD = NULL;

    return;
}


//check connection status
bool netbase::isConnected() const {
    return ready;
}


//Send packet on a socket descriptor 'sd'
int netbase::sendPacket( int sd, netpacket &msg) {

    int rv;
    const short length = msg.get_length();

    if (!ready) {
        debugLog << "Error: Disconnected, can't send" << endl;
        return -1;
    }

    //Check if connection number exists in conSet
    if ( conSet.find( sd ) == conSet.end() ) {
        debugLog << "Error: No socket " << sd << " to send on" << endl;
        return -1;
    }

    //Trying to send on socket
    rv = send(sd, (const char*)msg.get_ptr(), length, /*flags*/0);

    //TODO: while (rv > 0 && totalSent < length + NET_MSG_HEADER_BYTES
    if (rv == SOCKET_ERROR) {
        debugLog << "Socket Error:" << getSocketError() << endl;
        return -1;
    }
    
    //Record the message information, how much was sent
    debugLog    << "Sent, rv=" << rv << "/" << length << " bytes #" << sd << endl;
                
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
    std::set<int>::const_iterator iter;

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

//Receive all the data on incoming sockets.  Return number of packets processed.
int netbase::readIncomingSockets() {

    int rv=0;
    
    //Rebuild the client set, check for incoming data
    buildSocketSet();
    rv = select(sdMax+1, &sdSet, (fd_set *) 0, (fd_set *) 0, &timeout);

    if (rv == SOCKET_ERROR) {   //Socket select failed
        debugLog << "Socket select error:"  << getSocketError() << endl;
    }
    else if (rv == 0) {         //No new messages
        ;//debugLog << "No new server data" << endl;
    }
    else {                      //Something pending on a socket
        debugLog << "Incoming message"  << endl;
        rv = readSockets();
    }
    
    return rv;
}

//Check all sockets in sdSet for incoming data, then process callbacks
int netbase::readSockets()
{
    int rv=0, con=0;
    std::set<int>::const_iterator con_iter;
    vector< netpacket * > packets;
    
    //Copy connection set, as conSet entries may be deleted due to disconnects
    std::set<int> socketSet = conSet;

    //Check all connections in conSet
    for (con_iter = socketSet.begin(); con_iter != socketSet.end(); con_iter++) {
        con = *con_iter;
        if (FD_ISSET( con, &sdSet)) {
            //Data pending from client, get the message (or disconnect)
            size_t startIndex = myIndex;
            rv = recvSocket( con, (myBuffer + startIndex) );
            if ( rv > 0 ) {
            
                //Read from buffer into new packet object (don't forget to delete it)
                netpacket *pkt = getPacket( con, (myBuffer + startIndex), rv);
                
                //Set connection ID for packet
                pkt->ID = con;
                
                //Add packet to the queue
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
    map< int, netpacket::netPktCB >::const_iterator cb_iter;
    map< int, void* >::const_iterator cbd_iter;
    netpacket::netPktCB callback;
    void *cbData;
    
    for (pkt_iter = packets.begin(); pkt_iter != packets.end(); pkt_iter++) {

        //Packet pointer
        netpacket *pkt = *pkt_iter;
        if (pkt == NULL) {
            debugLog << "NULL packet in my queue??" << endl;
            continue;
        }
        
        //Perform the callback run for every incoming packet
        allCB( pkt, allCBD);
        
        //Get connection ID from packet (laaaazy)
        con = pkt->ID;
        
        //Connection specific callback data
        cbd_iter = packetCBD_map.find( con );
        if (cbd_iter == packetCBD_map.end()) {
            cbData = NULL;
        } else {
            cbData = cbd_iter->second;
        };
        
        //Find connection specific callback
        cb_iter = packetCB_map.find( con );
        
        //Run callback, if exists
        if ( cb_iter != packetCB_map.end()) {
            callback = cb_iter->second;
            callback( pkt, cbData); //What to do with return value?
        }
    }

    //Delete the dynamically created packet objects (but not what they point to)
    for (pkt_iter = packets.begin(); pkt_iter != packets.end(); pkt_iter = packets.begin()) {
        delete (*pkt_iter);
    }

  
    //If myIndex exceeds threshold, set it back to 0.
    if (myIndex > NET_MEMORY_SIZE) {
        myIndex = 0;
    }
    
    //Return number of packets processed (not total size)
    return packets.size();
}


//Recieve incoming data on a buffer, return the number of bytes read in
int netbase::recvSocket(int sd, unsigned char* buffer)
{
    int rv, rs;
    size_t offset = 0;

    //Create FD_SET that has only this socket
    fd_set sds, sds2;
    FD_ZERO(&sds);
    FD_SET((unsigned int)sd, &sds);

    do {
        //Read incoming bytes to buffer
        rv = recv( sd, (char*)(buffer + offset), NET_MAX_RECV_SIZE,  0 );
    
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
        debugLog << "recv " << rv << " bytes" << endl;
        offset += rv;

        //See if there is more to read
        sds2 = sds;
        rs = select(sd+1, &sds2, NULL, NULL, &timeout); 
        if (rs == SOCKET_ERROR) {   //Socket select failed
            debugLog << "Socket select error:"  << getSocketError() << endl;
        }
    } while (rs > 0);  //Keep reading if select says there's something here
    
    myIndex += offset;
    debugLog << "Received " << offset << " bytes on socket " << sd << endl;

    return rv;
}

//Default functions for function pointers
size_t netbase::incomingCB( netpacket* pkt, void *CBD) {
    if (pkt == NULL) {
#ifdef DEBUG
        std::cerr << "Null packet?" << endl;
#endif
        return -1;
    }
#ifdef DEBUG
    std::cerr << endl << "Packet on #" << pkt->ID << endl;
#endif
    return 0;
}

size_t netbase::connectionCB( int con, void *CBD) {

#ifdef DEBUG    
    std::cerr << endl << "New connection #" << con << endl;
#endif
    return 0;
};


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
    //string lastError;
    lastError = "Unknown network error";
    
#ifdef _WIN32
    switch ( WSAGetLastError()) {
        case WSANOTINITIALISED:
            lastError = "Server did not initialize WSA";
            break;
        case WSAENETDOWN:
            lastError = "Network failure";
            break;
        case WSAEADDRINUSE:
            lastError = "Address in use";
            break;
        case WSAEINTR:
            lastError = "Socket access interrupted";
            break;
        case WSAEINPROGRESS:
            lastError = "A socket is blocking";
            break;
        case WSAEADDRNOTAVAIL:
            lastError = "Address not available";
            break;
        case WSAEAFNOSUPPORT:
            lastError = "Address not supported";
            break;
        case WSAECONNREFUSED:
            lastError = "Connection (forcefully) refused";
            break;
        case WSAEDESTADDRREQ :
            lastError = "Address required";
            break;
        case WSAENOTCONN:
            lastError = "Socket already disconnected";
            break;
        case WSAEFAULT:
            lastError = "Programming mistake";
            break;
        case WSAEINVAL:
            lastError = "Unbound or invalid socket";
            break;
        case WSAEISCONN:
            lastError = "Already connected";
            break;
        case WSAEMFILE:
            lastError = "No more descriptors left";
            break;
        case WSAENETUNREACH:
            lastError = "Destination unreachable";
            break;
        case WSAENOBUFS:
            lastError = "Out of buffer space";
            break;
        case WSAENOTSOCK:
            lastError = "Non-socket descriptor";
            break;
        case WSAETIMEDOUT:
            lastError = "Connection timeout";
            break;
        case WSAEWOULDBLOCK:
            lastError = "Blocking on non-blocking socket";
            break;
        case WSAEOPNOTSUPP:
            lastError = "Unsupported socket operation";
            break;
        case WSAESHUTDOWN:
            lastError = "Socket already shutdown";
            break;
        case WSAEMSGSIZE:
            lastError = "Recieved oversized message";
            break;
        case WSAECONNABORTED:
            lastError = "Socket was lost";
            break;
        case WSAECONNRESET:
            lastError = "Socket was reset";
            break;
        default:
            lastError = "Unknown winsock error";
            break;
    }
#endif

    return lastError;
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
