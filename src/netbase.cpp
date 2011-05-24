// netbase: Create connections, send and receive data through them
//      Assign callbacks for netpackets that return true for specified function

//net__
#include "netbase.h"

//C++ IO
#include <iostream>
#include <iomanip>

//STL namespace
using std::map;
using std::pair;
using std::ofstream;
using std::string;
using std::vector;
using std::set;

using std::cerr;
using std::endl;
using std::hex;
using std::dec;
using std::flush;
using std::ios;
using std::setfill;
using std::setw;

//net__ namespace
using net__::netbase;
using net__::netpacket;

#ifdef _MSC_VER
#define snprintf _snprintf_s
#endif

//Constructor, specify the maximum client connections
netbase::netbase(size_t max): sdMax(-1), conMax( max),
    lastMessage(-1),  conCB(connectionCB), disCB(disconnectionCB)            
{

    //Assign callback data to this object
    conCBD = this;
    disCBD = this;

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
	int wsaret = WSAStartup(0x0202, &wsaData); //Request Winsock version 2.2
	if(wsaret!=0) {
        debugLog << "Error starting WSA" << endl;
        return;
    }
    if (wsaData.wVersion != 0x0202)             //Don't accept lower versions
    { // wrong WinSock version!
        debugLog << "Current winsock version "
            << hex << wsaData.wVersion << " unsupported" << endl << dec;
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
bool netbase::setConPktCB( sock_t c, netpacket::netPktCB cbFunc, void* cbData )
{
    bool result = true;

    //Map c -> cbFunc
    result = packetCB_map.insert(
        std::map< sock_t, netpacket::netPktCB >::value_type(c, cbFunc)).second;
    
    //Map c -> cbData
    result = result && packetCBD_map.insert(
        std::map< sock_t, void* >::value_type( c, cbData)).second;
    
    //Return value: was callback and callback data inserted successfully?
    return result;
}

//Remove callbacks for connection *c*
bool netbase::unsetConPktCB( sock_t c)
{
    bool result = true;
    
    //Unmap c -> callback function
    result = (packetCB_map.erase( c) > 0);
    
    //Unmap c -> callback data
    result = result && (packetCBD_map.erase( c) > 0);
    
    //Return value: was there a callback to remove?
    return result;
}

//Set callback for what to do when new connection arrives
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
bool netbase::isClosed(sock_t sd) const {
    return (conSet.count(sd) == 0);
}


//TODO: non-blocking send!
//  It should keep one big buffer that is send from asynchronously

//Send packet on a socket descriptor 'sd'.  Waits until send() completes
int netbase::sendPacket( sock_t sd, netpacket &msg) {

    int rv, readpos;
    const int length = msg.get_write();

    //Check if connection number exists in conSet
    if ( conSet.find( sd ) == conSet.end() ) {
        debugLog << "#" << sd << " socket not found for sendPacket()?" << endl;
        return -1;
    }

#ifdef DEBUG_PACKET
    debugPacket( &msg);
#endif

    //repeat send while (rv > 0 && totalSent < length)
    for ( readpos=0, rv=0; readpos < length; readpos += rv) {
        rv = send(sd, (const char*)(msg.get_ptr() + readpos), (int)length, 0);
        if (rv == SOCKET_ERROR || rv==-1) {
            debugLog << "#" << sd << " Error:" << getSocketError() << endl;
            return -1;
        }
    }
    
    //Record the message information, how much was sent
#ifdef DEBUG
    debugLog << "#" << sd << " sent "
            << rv << "/" << length << " bytes" << endl;
#endif

    if (rv == 0) {
        debugLog << "Warning: No data sent" << endl;
        return 0;
    }
    if ((size_t)rv < length ) {
        //TODO: keep a netpacket to be read from on the next send
        debugLog << "Warning: Send incomplete" << endl;
    }
    
    return rv;
}


//Setup a socket to be non-blocking and reusable
int netbase::unblockSocket(sock_t sd) {

    if (sd == (sock_t) INVALID_SOCKET) {
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
        debugLog << "#" << sd << " Error setting non-blocking socket" << endl;
        closeSocket(sd);
        return -1;
    }

#endif

    //Set socket lingering options (Don't linger... background will handle it).
    flags = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_DONTLINGER,
           (char *)&flags, sizeof(flags)) < 0) {
        debugLog  << "#" << sd << " Error for socket lingering" << endl;
        closeSocket(sd);
        return -1;
    }
    
    // set REUSEADDR / REUSEPORT flags
    flags = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,
           (char*) &flags, sizeof(flags)) < 0) {
        debugLog  << "#" << sd << " Error setting resusable address" << endl;
        closeSocket(sd);
        return -1;
    }

#ifdef SO_REUSEPORT
    flags = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEPORT,
        (char*) &flags, sizeof(flags)) < 0) {
        debugLog << "#" << sd << " Error setting reusable port" << endl;
        closeSocket(sd);
        return -1;
    }
#endif
    return 0;
}

//This creates an FD_SET from the server socket and all current client sockets
//This must be called before each new "select()" call
size_t netbase::buildSocketSet()
{
    std::set<sock_t>::const_iterator iter;

    FD_ZERO( &sdSet );
    for (iter = conSet.begin(); iter != conSet.end(); iter++) {
        FD_SET( (unsigned int)(*iter), &sdSet);
    }

    //Return number of sockets in set
    return conSet.size();
}


// Close a socket, remove it from the list of connections
int netbase::closeSocket(sock_t sd)
{
    int rv = removeSocket(sd);
    cleanSocket(sd);

    return rv;
}

int netbase::removeSocket(sock_t sd) {

    //Check if socket is already closed
    if (sd == (sock_t)INVALID_SOCKET) {
        debugLog << "Socket already closed" << endl;
        return sd;
    }

    //Close the socket
#ifdef _WIN32
    int rv = closesocket(sd);
#else
    int rv = close(sd);
#endif
    if (rv == SOCKET_ERROR) {
        debugLog << "#" << sd
            << " Error closing socket: " << getSocketError() << endl;
    }    

    //Remove this socket from the list of connected sockets
    //  Should have been done already!
    conSet.erase(sd);
    if (FD_ISSET((unsigned int)sd, &sdSet)) {
        FD_CLR((unsigned int)sd, &sdSet);
    }
    
    return rv;
}

//Remember this socket and disconnect it later.  Remove from sdSet and conSet!
void netbase::pendDisconnect(sock_t sd)
{
#ifdef DEBUG
    debugLog << "#" << sd << " pendDisconnect" << endl;
#endif

    //Remove this socket from the list of connected sockets
    conSet.erase(sd);
    if (FD_ISSET((unsigned int)sd, &sdSet)) {
        FD_CLR((unsigned int)sd, &sdSet);
    }

    //Remember to free the buffer for this socket later
    closedSocketSet.insert(sd);

}

//Clean up data associated with socket
void netbase::cleanSocket(sock_t sd) {

#ifdef DEBUG
    debugLog << "#" << sd << " cleanSocket" << endl;
#endif

    //Remove the callbacks associated with this socket
    unsetConPktCB(sd);

    //Free the buffer allocated for this socket
    if (conBuffer[sd] != NULL) {
        delete conBuffer[sd];
        conBuffer[sd] = NULL;
    }
    
    //Remove this socket from the set of sockets to be closed
    closedSocketSet.erase(sd);

    //Reset index/length/size pointers
    conBufferIndex[sd] = 0;
    conBufferLength[sd] = 0;
    conBufferSize[sd] = 0;
}

//Disconnect specific connection
bool netbase::disconnect( sock_t con) {

    bool result=false;
    
    //If it's not an invalid socket and not closed already
    if (con != (sock_t)(INVALID_SOCKET) && conSet.count(con) != 0) {
        result = (closeSocket(con) != SOCKET_ERROR);
        char socketNum[8];
        snprintf(socketNum, 8, "%d", con);
        lastError = lastError + string("; Closing socket #") + socketNum;
    } else {
        debugLog << "#" << con << " was already disconnected" << endl;
    }
    
    //Unset the connection specific callback
    unsetConPktCB( con );
    
    return result;
}

//Read all incoming data, then fire callbacks
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
        vector<netpacket*> packets = readSockets();
        rv = fireCallbacks(packets);
    }
    //****DEBUG****
    //cerr << "*";

    return rv;
}

//Check all sockets in sdSet for incoming data, then process callbacks
vector<netpacket*> netbase::readSockets()
{
    int rv=0, con=0;
    std::set<sock_t>::const_iterator con_iter;
    vector< netpacket * > packets;
    
    //Copy connection set, as conSet entries may be deleted due to disconnects
    std::set<sock_t> socketSet = conSet;

    //Check all connections in conSet
    for (con_iter = socketSet.begin(); con_iter!=socketSet.end(); con_iter++) {
        con = *con_iter;
        if (FD_ISSET( con, &sdSet)) {
          
            //Get offset in connection buffer
            size_t bufferOffset = conBufferLength[con];

            //Resize connection buffer, if needed
            uint8_t* myBuffer=NULL;
            while (bufferOffset + netbase::NETMM_MAX_RECV_SIZE >
                    conBufferSize[con])
            {
                //Increase connection buffer size
                conBufferSize[con] = (conBufferSize[con] << 1);
                
                //Increase memory allocation
                if (conBufferSize[con] == 0) {
                    //Buffer was unallocated... better create one.
                    conBufferSize[con] = netbase::NETMM_MAX_RECV_SIZE;
                    conBuffer[con] = new uint8_t[conBufferSize[con]];
                } else {
                
                    //Copy old buffer to bigger buffer
                    myBuffer = new uint8_t[conBufferSize[con]];
                    memcpy( myBuffer, conBuffer[con], bufferOffset);
                    
                    //Delete old buffer
                    delete conBuffer[con];
                    conBuffer[con] = myBuffer;
                }
            }
            
            //Point to connection buffer
            myBuffer = conBuffer[con];

            //Copy the incoming bytes to myBuffer + offset
            rv = recvSocket( con, (myBuffer + bufferOffset) );

            //Point the packet object at new received bytes
            if ( rv > 0 ) {

                //Keep track of buffer offsets
                conBufferLength[con] += rv;
                
                //Packet points at unconsumed buffer space
                netpacket *pkt = makePacket(con,myBuffer + conBufferIndex[con],
                    conBufferLength[con] - conBufferIndex[con]);
                
                //Set connection ID for packet
                pkt->ID = con;
                                
                //Add packet to the queue
                packets.push_back( pkt);

                //DEBUG
                debugLog << "#" << con << " Added packet size=" 
                        << conBufferLength[con] - conBufferIndex[con] << endl;

            }
        }
    }

    return packets;
}

//Fire associated callbacks for the list of netpackets
int netbase::fireCallbacks( vector<netpacket*>& packets) {
  
    //All pending data has been read, fire incoming data callbacks now
    vector< netpacket * >::iterator pkt_iter;
    map< sock_t, netpacket::netPktCB >::const_iterator cb_iter;
    map< sock_t, void* >::const_iterator cbd_iter;
    netpacket::netPktCB callback;
    void *cbData;

    size_t bytes_read;
    sock_t con;
    
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

            //Keep running callback until no more bytes are read(?)
            do {
                bytes_read = callback( pkt, cbData);
                conBufferIndex[con] += bytes_read;
#ifdef DEBUG
                debugLog << "#" << con << " bytes_read=" << bytes_read
                    << " index=" << conBufferIndex[con] << " length="
                    << conBufferLength[con] << endl;
#endif
                //Reset buffer if all data has been consumed
                if (conBufferIndex[con] == conBufferLength[con]) {
                    conBufferIndex[con] = 0;
                    conBufferLength[con] = 0;
                } else if (conBufferLength[con] < conBufferIndex[con]) {
                    //Index should never go past length.
                    debugLog << "#" << con
                        << " ERROR! Read past end of packet "
                        << conBufferIndex[con] << "/" << conBufferLength[con]
                        << endl;
                    cerr << "#" << con << " ERROR! Read past end of packet "
                        << conBufferIndex[con] << "/" << conBufferLength[con]
                        << endl;
                    cerr << "bytes_read=" << bytes_read << " Index was "
                        << (int)(conBufferIndex[con] - bytes_read)
                        << " first byte=0x" << hex
                        << (int)(conBuffer[(conBufferIndex[con] - bytes_read)])
                        << dec << endl;
                    
                    //Set index back to max length and quit
                    conBufferIndex[con] = conBufferLength[con];
                } else if (bytes_read > 0) {
                    //Make a new packet, run the callback again.
                    //  There may be more messages after the single one read in
                    delete pkt;
                    pkt = makePacket(con,
                        conBuffer[con] + conBufferIndex[con],
                        conBufferLength[con] - conBufferIndex[con]);
                    pkt->ID = con;
                    *pkt_iter = pkt;
                }
                //cerr << "-";
            } while (bytes_read > 0 &&
                conBufferIndex[con] < conBufferLength[con]);
        } else {
            debugLog << "#" << con << " no connection callback!" << endl;
        }
    }

    //Handle disconnected sockets
    //  (this can happen immediately after receiving bytes)
    set<sock_t>::const_iterator con_iter;
    set<sock_t> closedSocketCopy( closedSocketSet );
    for (con_iter = closedSocketCopy.begin();
         con_iter != closedSocketCopy.end();
         con_iter++)
    {


        con = *con_iter;
        debugLog << "#" << con << " disconnect callback" << endl;

        //Disconnection callback
        disCB( con, disCBD);
        
        //Actually close the socket, and clean up associated data
        closeSocket(con);
    }


    //Delete packets created by makePacket() in readSockets().
    //  Create list of connections with unread data
    for (pkt_iter = packets.begin(); pkt_iter != packets.end(); pkt_iter++) {
        delete (*pkt_iter);
    }
      
    //Return number of packets processed (not total size)
    return (int)(packets.size());
}

//Recieve incoming data on a buffer, return the number of bytes read in
//Check if socket is closed after receiving
int netbase::recvSocket(sock_t sd, uint8_t* buffer)
{
    int rv, rs;
    int offset = 0;

    //Create FD_SET that has only this socket
    fd_set sds, sds2;
    FD_ZERO(&sds);
    FD_SET((unsigned int)sd, &sds);

    do {
        //Read incoming bytes to buffer
        rv = recv( sd, (char*)(buffer + offset), NETMM_MAX_RECV_SIZE,  0 );
    
        if (rv == 0) {
#ifdef DEBUG
            debugLog << "#" << sd << " disconnected from us" << endl;
#endif
            pendDisconnect(sd);
            rs=0;   //Disconnected, nothing more to receive...
            //If we got anything on an earlier loop iteration,
            //  we will need to process it.
            break;
        }
        
        //Check for reset socket, which we treat as normal disconnect
        //      since we are lazy
        #ifdef _WIN32
        if (WSAGetLastError() == WSAECONNRESET) {
            debugLog << "#" << sd << " reset" << endl;
            pendDisconnect(sd);
            return 0;
        }
        #endif
        
        if (rv == SOCKET_ERROR) {
            debugLog << "#" << sd << " recv Error: "<< getSocketError()<< endl;
            pendDisconnect(sd);
            return -1;
        }
    
        //rv was not 0 or SOCKET_ERROR, so it's the number of bytes received.
#ifdef DEBUG
        debugLog << "#" << sd << " recv " << rv << " bytes" << endl;
#endif
        offset += rv;

        //See if there is more to read
        sds2 = sds;
        
        //FIRST argument is highest socket descriptor + 1
        //SECOND argument is FD_SET containing socket(s) to read
        //THIRD argument is FD_SET containing socket(s) to write
        //FOURTH argument is FD_SET containing out-of-band socket data
        //FIFTH argument is timeout until select stops blocking
        rs = select(sd+1, &sds2, NULL, NULL, &timeout); 
        
        if (rs == SOCKET_ERROR) {   //Socket select failed
            debugLog << "select() error:"  << getSocketError() << endl;
        }
    } while (rs > 0);  //Keep reading if select says there's something here
    
    if (offset > 0) {
#ifdef DEBUG
        debugLog << "#" << sd << " Received " << offset << " bytes" << endl;
#endif
    }

    return offset;  //Return total number of bytes received
}

//Default functions for function pointers.  Your replacement must return
//  number of bytes consumed.
size_t netbase::incomingCB( netpacket* pkt, void *CBD) {
  
    
    if (pkt == NULL) {
#ifdef DEBUG
        std::cerr << "Null packet?" << endl;
#endif
        return -1;
    }

#ifdef DEBUG
    if (CBD == NULL) {
        std::cerr << "Null callback data on incomingCB!" << endl;
        std::cerr << "Packet on #" << pkt->ID << endl;
    } else {
        ((netbase*)CBD)->debugLog << "#" << pkt->ID << " maxsize="
            << pkt->get_maxsize() << endl;
    }
#endif

    //Don't use any bytes from the packet...
    return 0;
}

//Default new incoming connection callback
size_t netbase::connectionCB( sock_t con, void *CBD) {

#ifdef DEBUG
    if ( CBD == NULL) {
        std::cerr << "Null callback data on connectionCB!" << endl;
        std::cerr << "New connection #" << con << endl;
    } else {
        ((netbase*)CBD)->debugLog << "#" << con << " connected" << endl;
    }
#endif
    return con;
};

//Default disconnect callback
size_t netbase::disconnectionCB( sock_t con, void *CBD) {
#ifdef DEBUG
    if ( CBD == NULL) {
        std::cerr << "Null callback data on disconnectionCB!" << endl;
        std::cerr << "Dropped connection #" << con << endl;
    } else {
        ((netbase*)CBD)->debugLog << "#" << con << " disconnectCB" << endl;
    }
#endif
    return con;
};


//Output the contents of a buffer to the log
void netbase::debugBuffer( uint8_t* buffer, size_t buflen) const
{
    //int byte, width=16;
    size_t byte, width=16;
    uint16_t curr_word;

    debugLog << "::: Buffer contents ::: " << endl << hex;

    //Dump the buffer contents
    for (byte = 0; byte < buflen - 1; byte += 2) {

        memcpy( &curr_word, (buffer + byte), sizeof(curr_word));
        debugLog << "0x" << setfill('0') << setw(4) << curr_word;

        if ((byte % width ) == (width - 2))
            debugLog << endl;
        else
            debugLog << " ";
    }

    //If it is off a word boundary, dump the last byte
    if (byte == buflen - 1)
        debugLog << " 0x" << hex << setw(2) << (uint16_t)buffer[byte] << dec;

    if ( (byte % width) > 0)
        debugLog << endl;
}

//New packet object, pointing at the data in a connectin-specific buffer
netpacket *netbase::makePacket( sock_t con, uint8_t *buffer, size_t pkt_size)
{
    netpacket *result = new netpacket( pkt_size, buffer, 0 );
    result->ID = con;
#ifdef DEBUG_PACKET
    debugPacket(result);
#endif
    
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
    //Only in debug mode!
#ifndef DEBUG
    return 0;
#endif
    if (pkt == NULL) {
        debugLog << "Null packet" << endl;
        return 0;
    }
    
    //Get index values
    size_t read = pkt->get_read();
    size_t written = pkt->get_write();
    size_t size = pkt->get_maxsize();
    debugLog << "#" << pkt->ID << " (w=" << written << ","
             << "r=" << read << "/" << size << ")" << endl;

    const uint8_t *mybytes = pkt->get_ptr();
    uint8_t mybyte;
    char hexvalue[8];

    //Print the written bytes    
    if (written > 0) {
        debugLog << "     Written=";
        //Vars
        
        for (size_t index=0; index < written; index++) {
            mybyte = mybytes[index];
            if (mybyte >= ' ' && mybyte <= '~') {
                debugLog << (char)mybyte;
            } else {
                snprintf( hexvalue, 8, "{0x%02X}", mybyte);
                debugLog << hexvalue;
            }
        }
        debugLog << endl;
    } else if (read < size) {
        debugLog << "     Unread=";
        //Print the unread bytes
        for (size_t index=read; index < (size - read); index++) {
            mybyte = mybytes[index];
            if (mybyte >= ' ' && mybyte <= '~') {
                debugLog << (char)mybyte;
            } else {
                snprintf( hexvalue, 8, "{0x%02X}", mybyte);
                debugLog << hexvalue;
            }
        }
        debugLog << endl;
    }
    
    //Did not consume any bytes :)
    return 0;
}
