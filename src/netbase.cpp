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
    lastMessage(-1), 
    conCB(connectionCB), conCBD(this), disCB(disconnectionCB), disCBD(this)
{

    //Set all buffer pointers in map to null
    for (size_t sd = 0; sd < NETMM_MAX_SOCKET_DESCRIPTOR; sd++)
    {
        conBuffer[sd] = NULL;
        conBufferIndex[sd] = 0;
        conBufferLength[sd] = 0;
        conBufferSize[sd] = 0;
    }

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

    debugLog << "~netbase" << endl;

    //Free space from open connections
    for (size_t sd = 0; sd < NETMM_MAX_SOCKET_DESCRIPTOR; sd++)
    {
        if (conBuffer[sd] != NULL) {
            delete conBuffer[sd];
            conBuffer[sd] = NULL;
        }
    }
}

//Add a callback for incoming packets on matching connection *c*
bool netbase::setConPktCB( int c, netpacket::netPktCB cbFunc, void* cbData )
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
bool netbase::unsetConPktCB( int c)
{
    bool result = true;
    
    //Unmap c -> callback function
    result = (packetCB_map.erase( c) > 0);
    
    //Unmap c -> callback data
    result = result && (packetCBD_map.erase( c) > 0);
    
    //Return value: was there a callback to remove?
    return result;
}

//Set callback for what to do when new connection is created
void netbase::setConnectCB( connectionFP cbFunc, void *cbData)
{
    conCB = cbFunc;
    conCBD = cbData;
}

//Add disconnection callback
void netbase::setDisconnectCB( connectionFP cbFunc, void *cbData )
{
    //Remember disconnect callback info
    disCB = cbFunc;
    disCBD = cbData;
}

//Remove disconnection callback
void netbase::removeDisconnectCB()
{
    disCB = disconnectionCB;
    disCBD = this;
}

//Clear the generic and connection specific incoming packet callbacks
void netbase::unsetAllPktCB()
{
    //Affects connection specific callbacks
    packetCB_map.clear();
    packetCBD_map.clear();

    return;
}


//See if socket is still in connection set
bool netbase::isClosed(int sd) const {
    return (conSet.count(sd) == 0);
}

//Send packet on a socket descriptor 'sd'
int netbase::sendPacket( int sd, netpacket &msg) {

    int rv;
    const short length = msg.get_position();

    //Check if connection number exists in conSet
    if ( conSet.find( sd ) == conSet.end() ) {
        debugLog << "Error: No socket " << sd << " to send on" << endl;
        return -1;
    }

    //Trying to send on socket. TODO: repeat while (rv > 0 && totalSent < length)
    rv = send(sd, (const char*)msg.get_ptr(), length, /*flags*/0);

    //Return on socket errors
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
    for (iter = conSet.begin(); iter != conSet.end(); iter++) {
        FD_SET( (unsigned int)(*iter), &sdSet);
    }

    //Return number of sockets in set
    return conSet.size();
}


// Close a socket, remove it from the list of connections
int netbase::closeSocket(int sd)
{
    int rv;

    //Check if socket is already closed
    openLog();
    if (sd == (int)INVALID_SOCKET) {
        debugLog << "Socket is already closed" << endl;
        return sd;
    }

    //Close the socket
    debugLog << "Closing socket " << sd << endl;
    rv = closesocket(sd);
    if (rv == SOCKET_ERROR) {
        debugLog << "Error closing socket " << sd << ":" << getSocketError() << endl;
    }
    
    removeSocket(sd);
    cleanSocket(sd);

    return rv;
}

void netbase::removeSocket(int sd) {
    //Remove this socket from the list of connected sockets
    conSet.erase(sd);
    if (FD_ISSET((unsigned int)sd, &sdSet)) {
        FD_CLR((unsigned int)sd, &sdSet);
    }
}

//Clean up data associated with socket
void netbase::cleanSocket(int sd) {

    //Free the buffer allocated for this socket
    if (conBuffer[sd] != NULL) {
        delete conBuffer[sd];
        conBuffer[sd] = NULL;
    }

    //Reset index/length/size pointers
    conBufferIndex[sd] = 0;
    conBufferLength[sd] = 0;
    conBufferSize[sd] = 0;
}

//Disconnect specific connection
bool netbase::disconnect( int con) {

    bool result=false;
    
    if (con != (int)(INVALID_SOCKET)) {
        result = (closeSocket(con) != SOCKET_ERROR);
        char socketNum[8];
        sprintf(socketNum, "%d", con);
        lastError = lastError + string("; Closing socket #") + socketNum;
    }
    
    return result;
}

//Receive all the data on incoming sockets.  Return number of packets processed.
int netbase::readIncomingSockets() {

    int rv=0;
    
    //Rebuild the socket set, check for incoming data
    if (buildSocketSet() == 0) {
        debugLog << "No connections to read" << endl;
        return 0;
    }
    
    //Check socket set for waiting data, until timeout passes
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

    //TODO: loop select statement, then fire callbacks
    
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
          
            //Get offset in connection buffer
            size_t bufferOffset = conBufferLength[con];

            //Resize connection buffer, if needed
            unsigned char* myBuffer=NULL;
            if (bufferOffset + netbase::NETMM_MAX_RECV_SIZE > conBufferSize[con]) {
                //Increase connection buffer size
                conBufferSize[con] = (conBufferSize[con] << 1);
                
                //Copy old buffer to bigger buffer
                myBuffer = new unsigned char[conBufferSize[con]];
                memcpy( myBuffer, conBuffer[con], bufferOffset);
                
                //Delete old buffer
                delete conBuffer[con];
                conBuffer[con] = myBuffer;
            } else {
                //Use existing buffer
                myBuffer = conBuffer[con];
            }

//TODO: move packet logic to readIncomingSockets

            //Copy the incoming bytes to myBuffer + offset
            rv = recvSocket( con, (myBuffer + bufferOffset) );

            //Point packet object at received bytes
            if ( rv > 0 ) {

                //Keep track of buffer offsets
                conBufferLength[con] += rv;
                
                //Packet points at entire unconsumed buffer space
                netpacket *pkt = makePacket( con, (myBuffer + conBufferIndex[con]), conBufferLength[con]);
                
                //Set connection ID for packet
                pkt->ID = con;
                                
                //Add packet to the queue
                packets.push_back( pkt);
            }
        }
    }
    
    //All pending data has been read, fire incoming data callbacks now
    vector< netpacket * >::const_iterator pkt_iter;
    map< int, netpacket::netPktCB >::const_iterator cb_iter;
    map< int, void* >::const_iterator cbd_iter;
    netpacket::netPktCB callback;
    void *cbData;
    
    //For each packet on the list
    for (pkt_iter = packets.begin(); pkt_iter != packets.end(); pkt_iter++) {

        //Packet pointer
        netpacket *pkt = *pkt_iter;
        if (pkt == NULL) {
            debugLog << "NULL packet in my queue??" << endl;
            continue;
        }

        //Get connection ID from packet (I hope we added it earlier!)
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
        
        //Run connection specific callback, if exists
        if ( cb_iter != packetCB_map.end()) {
            callback = cb_iter->second;
            
            //Increment buffer index to start of unprocessed data
            conBufferIndex[con] += callback( pkt, cbData);
            
            //Reset buffer if all data has been consumed
            if (conBufferIndex[con] == conBufferLength[con]) {
                conBufferIndex[con] = 0;
                conBufferLength[con] = 0;
            }
        }
    }

    //Handle disconnected sockets (this can happen immediately after receiving bytes)
    for (con_iter = socketSet.begin(); con_iter != socketSet.end(); con_iter++) {
        con = *con_iter;
    
        //If the connection is closed...
        if (isClosed(con)) {
            //Disconnection callback
            disCB( con, disCBD);
            
            //Clean up the socket associated data
            cleanSocket(con);
        }
    }


    //Delete the dynamically created packet objects (but not what they were point to)
    for (pkt_iter = packets.begin(); pkt_iter != packets.end(); pkt_iter++) {
        delete (*pkt_iter);
    }
      
    //Return number of packets processed (not total size)
    return packets.size();
}


//Recieve incoming data on a buffer, return the number of bytes read in
//Check if socket is closed after receiving
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
        rv = recv( sd, (char*)(buffer + offset), NETMM_MAX_RECV_SIZE,  0 );
    
        if (rv == 0) {
            debugLog << "Socket " << sd << " disconnected, remove now, clean later" << endl;
            removeSocket(sd);
            rs=0;   //Disconnected, nothing more to receive...
            //If we got anything on an earlier loop iteration, will need to process it.
            break;
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
    
    debugLog << "Received " << offset << " bytes on socket " << sd << endl;

    return offset;  //Return total number of bytes received
}

//Default functions for function pointers.  Your replacement must return number of bytes consumed.
size_t netbase::incomingCB( netpacket* pkt, void *CBD) {
  
    
    if (pkt == NULL) {
#ifdef DEBUG
        std::cerr << "Null packet?" << endl;
#endif
        return -1;
    }

    if (CBD == NULL) {
#ifdef DEBUG
        std::cerr << "Null callback data on incomingCB!" << endl;
        std::cerr << "Packet on #" << pkt->ID << endl;
#endif
    } else {
        ((netbase*)CBD)->debugLog << "Packet on #" << pkt->ID << " len=" << pkt->get_maxsize() << endl;
    }

    //Don't use any bytes from the packet...
    return 0;
}

//Default new incoming connection callback
size_t netbase::connectionCB( int con, void *CBD) {

#ifdef DEBUG
    if ( CBD == NULL) {
        std::cerr << "Null callback data on connectionCB!" << endl;
        std::cerr << "New connection #" << con << endl;
    } else {
        ((netbase*)CBD)->debugLog << "New connection #" << con << endl;
    }
#endif
    return con;
};

//Default disconnect callback
size_t netbase::disconnectionCB( int con, void *CBD) {
#ifdef DEBUG
    if ( CBD == NULL) {
        std::cerr << "Null callback data on disconnectionCB!" << endl;
        std::cerr << "Dropped connection #" << con << endl;
    } else {
        ((netbase*)CBD)->debugLog << "Dropped connection #" << con << endl;
    }
#endif
    return con;
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
netpacket *netbase::makePacket( int con, unsigned char *buffer, short len)
{
    netpacket *result = new netpacket( len, buffer, 0 );
    result->ID = con;
    debugPacket(result);
    
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

//Print contents of packet.  Wrap unprintable bytes in { }
size_t netbase::debugPacket(const netpacket *pkt) const
{

    if (pkt == NULL) {
        debugLog << "Null packet" << endl;
        return 0;
    }
    
    //Useful size depends if the packet is being sent, or received
    size_t position = pkt->get_position();
    size_t size = pkt->get_maxsize();
    size_t length = (position == 0 ? size : position);
    
    debugLog << "Length=" << length << " ID=" << pkt->ID << " String=";
    const unsigned char *mybytes = pkt->get_ptr();
    unsigned char mybyte;
    char hexvalue[8];
    
    for (size_t index= 0; index < length; index++) {
        mybyte = mybytes[index];
        if (mybyte >= ' ' && mybyte <= '~') {
            debugLog << (char)mybyte;
        } else {
            sprintf( hexvalue, "{0x%02X}", mybyte);
            debugLog << hexvalue;
        }
    }
    
    debugLog << endl;
    
    //Did not consume any bytes :)
    return 0;
}
