
//netServer: listens for incoming connections, handles incoming data.
//  Derived your class from this, and override handleIncoming( netpacket *pkt)

#include "netserver.h"
#include <iostream>
#include <iomanip>

using std::cerr;
using std::endl;
using std::hex;
using std::dec;
using std::ios;
using std::setfill;
using std::setw;

//
//  Function implementations
//


//Constructor, specify the maximum client connections
netserver::netserver(unsigned int max): netbase( max), ready(false),
    serverPort(-1), sdListen(INVALID_SOCKET)
{
    //Everything else should have been taken care of by the netbase constructor
    openLog();
    debugLog << "===Starting server===" << endl;
    FD_ZERO( &listenSet);
}

//Destructor... was virtual
netserver::~netserver()
{
    if ( (sdListen != (sock_t)INVALID_SOCKET) || ready)
        closePort();
    openLog();
    debugLog << "===Ending server===" << endl << endl;
    closeLog();
    //Now the base class destructor is invoked by C++
}


//Open a socket, bind, and listen on specified port
sock_t netserver::openPort(int16_t port) {
	sock_t sd;	 // socket descriptors
	struct sockaddr_in sad; // structure to hold server's address
	int rv;    //Return value

    if (ready)
        debugLog << "Warning: Opening port while server ready" << endl;
    ready = false;  //Assume not ready until port is opened

    //Restart the log if it was closed
    openLog();

    //Get a socket
    sd = socket( AF_INET, SOCK_STREAM, 0);
    if ( sd == (sock_t)INVALID_SOCKET )
    {
        debugLog << "#" << sd << " socket error: " << getSocketError() << endl;
        return -1;
    }

    if (unblockSocket( sd) < 0)
        return -1;

	memset((char *)&sad,0,sizeof(sad)); // clear sockaddr structure
	sad.sin_family = AF_INET;	  // set family to Internet
	sad.sin_addr.s_addr = INADDR_ANY; // set the local IP address
	sad.sin_port = htons(port);

	rv = bind(sd, (sockaddr*)&sad, sizeof( struct sockaddr_in));
	if (rv == -1) {
        debugLog << "#" << sd << " cannot bind localhost:" << port << endl;
        closeSocket(sd);
        return -1;    //Cannot bind socket
    }

    rv  = listen(sd, conMax);    //List on port... allow MAX_CON clients
    if (rv == -1) {
        debugLog << "#" << sd << " cannot listen on " << port << endl;
        closeSocket(sd);
        return -1;    //Cannot listen on port
    }

    //Add to sdSet
    FD_SET( (unsigned int)sd, &sdSet);
    if (sdMax < sd)
        sdMax = sd;

    //Set the class members
    sdListen = sd;
    serverPort = port;
    ready = true;

    debugLog << "#" << sd << " ** Listening on port " << port
                << ", limit " << conMax << " clients **" << endl;

    return sd;
}

//Stop listening on the port
void netserver::closePort()
{
    openLog();
    if (ready)
        debugLog << "#" << sdListen << " Server port " << serverPort << " closing." << endl << endl;
    else
        debugLog << "#" << sdListen << " Warning: Closing port, may already be closed!" << endl;

    ready = false;
    
    //Close the socket.  ready MUST be set to FALSE or there will be a loop
    if (sdListen != (sock_t)INVALID_SOCKET)
        closeSocket( sdListen);
    sdListen = (sock_t)INVALID_SOCKET;
    
    closeLog();
    
}

//Inherited function for closing a network socket
int netserver::closeSocket( sock_t sd)
{
    int rv;

    //Do normal base class closing
    rv = netbase::closeSocket( sd);

    //Special test case for listening port
    if (sd == sdListen) {
        sdListen = (sock_t)INVALID_SOCKET;
    }
    return rv;
}

//Read the network, handle any incoming data
int netserver::run()
{
    int rv=0;

    try {
    
        //First, look for incoming connections
        checkPort();

        //We have more than 0 clients, check for data
        if (conSet.size() > 0) {
            rv = readIncomingSockets();
        }
        
        //Fire callbacks for unprocessed data
    }
    catch(...) {
        debugLog << "Unhandled exception!!" << endl;
        rv = -1;
    };

    return rv;
}

//This creates an FD_SET from the server port listening sockets only
int netserver::buildListenSet()
{
    FD_ZERO( &listenSet );
    FD_SET( (unsigned int)sdListen, &listenSet );

    return 0;
}

//Check sdListen for new incoming connections
int netserver::checkPort()
{
    sock_t sd=0, rv;

    //Rebuild the server socket set
    buildListenSet();
    
    //Check each listen socket for incoming data
    rv = select(sdListen+1, &listenSet, (fd_set *) 0, (fd_set *) 0, &timeout);
    
    if (rv == SOCKET_ERROR) {
        //Socket select failed with error
        debugLog << "Listen select error:"  << getSocketError() << endl;
    }
    else if (rv > 0)
    {
        //Something pending on a socket

        //Check the incoming server socket (dedicated to listening for new connections)
        if (FD_ISSET(sdListen, &listenSet)) {
            sd = acceptConnection();
            
            if (sd == (sock_t)INVALID_SOCKET) {
                //Connection refused or failed
            }
            else {
                //Report new connection
                debugLog << "#" << sd << " CONNECTED!" << endl;
                conCB( sd, conCBD);
            }
        } else {
          ;//Existing connection has something to say
        }
    }
    
    return rv;  //Return value of select statement...
}

//Accept new connection, add connection descriptor to conSet
sock_t netserver::acceptConnection()
{
    sock_t sd;
    struct sockaddr_in addr;
    int addr_len = sizeof(struct sockaddr_in);

    openLog();

    sd = accept(sdListen, (sockaddr*)&addr, &addr_len);        //Non blocking accept call
    if (sd == (sock_t)INVALID_SOCKET) {
        debugLog << "Client connection failed:" << getSocketError() << endl;

        if (sdListen != (sock_t)INVALID_SOCKET) {
            closeSocket(sdListen);  //Cleanup the listen port
        }
        
        //Don't give up, try to restart it
        if (openPort(serverPort) == (sock_t)INVALID_SOCKET)
            debugLog << "Cannot restart socket!" << endl;
        else
            debugLog << "Listening socket restarted" << endl;
        return -1;
    }

    if (conSet.size() >= conMax) {
        debugLog << "Connection refused, maximum " << conMax << " connections" << endl;
        return -1;
    }
    debugLog << "#" << sd
            << " connected!  address=" << inet_ntoa( addr.sin_addr )
            << "  port=" << ntohs(addr.sin_port) << endl;

    //Add to the set of connection descriptors
    conSet.insert( sd );
    
    //Allocate buffer for receiving packets
    conBuffer[sd] = new uint8_t[NETMM_CON_BUFFER_SIZE];
    conBufferIndex[sd] = 0;
    conBufferLength[sd] = 0;
    conBufferSize[sd] = NETMM_CON_BUFFER_SIZE;

    //unblockSocket( connection );
    //Do something with FD_SET ???

    return sd;
}
