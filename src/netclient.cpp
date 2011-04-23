#include "netclient.h"

//STL namespace
using std::string;
using std::endl;

//net__ namespace
using net__::netbase;
using net__::netclient;

//
//  netclient function implementations
//

// Constructor: Set maximum connections
netclient::netclient( size_t max ) : netbase( max)
{
    openLog();
    debugLog << "===Starting client===" << endl;
    
    //Set the timeout for connecting to a server...
    connTimeout.tv_sec = 3;
    connTimeout.tv_usec = 0;
}

// Destructor
netclient::~netclient()
{
    openLog();
    debugLog << "===Ending client===" << endl << endl;
    closeLog();

    //Now the base class destructor is invoked by C++
}

// Set timeout for initial connection
bool netclient::setConnTimeout( int seconds, int microsec)
{
    //TODO: make sure the input is reasonable

    connTimeout.tv_sec = seconds;
    connTimeout.tv_usec = microsec;
    
    return true;
}


//connect to server "address:port", return socket number
sock_t netclient::doConnect(const string& serverAddress,
                            uint16_t port, uint16_t lport)
{
	struct	sockaddr_in sad;   //Server address struct
	sock_t sdServer;            //socket descriptor of connection
	int rv;                    //Return value
    struct hostent *host;       //Host name entry

    openLog();

    //Create a socket
    sdServer = socket( AF_INET, SOCK_STREAM, 0);
    if ( sdServer == (sock_t)INVALID_SOCKET ) {
        lastError = "Could not create socket";
        debugLog << lastError << endl;
        return -1;
    }

    //Set socket to be non-blocking
    if (unblockSocket( sdServer) < 0)
        return -1;

    //
    //Load connection information
    //
    
    // clear sockaddr structure
	memset((char *)&sad,0,sizeof(sad));
	
	//Set family to Internet
	sad.sin_family = AF_INET;
	
	//Set port (network int16_t format)
	sad.sin_port = htons(port);
		
	//Convert the server address
	sad.sin_addr.s_addr = inet_addr(serverAddress.c_str());

    //If not a numeric address, resolve it
	if (sad.sin_addr.s_addr == INADDR_NONE)
	{
        host = NULL;    //...
        host = gethostbyname(serverAddress.c_str());
        if (host == NULL) {
          
            //Set error message
            lastError = string("Unknown host ") + serverAddress.c_str();
            debugLog << lastError << endl;
            
            //Cleanup the socket
            closeSocket(sdServer);
            return -1;
        }
        memcpy( &sad.sin_addr, host->h_addr_list[0], host->h_length);
	}


    //Connect to the server
	rv = connect(sdServer, (const struct sockaddr*)&sad,
                 sizeof( struct sockaddr_in));

    //Special case for blocking sockets
	if (rv == SOCKET_ERROR) {
        #ifdef WIN32
        fd_set sdConnSet;
        FD_ZERO(&sdConnSet);
        FD_SET( (unsigned int)sdServer, &sdConnSet);
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
             rv = select(sdMax+1, (fd_set *) 0, &sdConnSet,
                         (fd_set *) 0, &connTimeout);
             if (rv == 0) {
                debugLog << "Timeout to " << serverAddress << endl;
                lastError = "Timed out: " + serverAddress;
                return SOCKET_ERROR;      //Try again later
            }
        }
        #endif
    }
    
    //Check for error
    if (rv == SOCKET_ERROR) {
      
        //Update error message
        debugLog << "   " << getSocketError() << endl;
        lastError = string("Could not connect to ") + serverAddress;
        debugLog << lastError << endl;
        
        //Cleanup the socket
        closeSocket(sdServer);
        
        //Some problem connecting to the server
        return SOCKET_ERROR;    
    }
    else if (rv > 0) {
      
        //namelen must be an "int"
        int namelen = sizeof( struct sockaddr_in);
        getsockname( sdServer, (struct sockaddr*)&sad, &namelen);
        
        //Write to debug log
        debugLog << "#" << sdServer << " connected @ "
                 << serverAddress << ":" << port
                << " from " << inet_ntoa(sad.sin_addr) 
                << ":" << ntohs(sad.sin_port) << endl;

        //Add to sdSet
        conSet.insert(sdServer);
        buildSocketSet();
        
        //Allocate buffer for receiving packets
        conBuffer[sdServer] = new uint8_t[NETMM_CON_BUFFER_SIZE];
        conBufferIndex[sdServer] = 0;
        conBufferLength[sdServer] = 0;
        conBufferSize[sdServer] = NETMM_CON_BUFFER_SIZE;

        //Increase sdMax if higher connection is made
        if (sdMax < sdServer) {
            sdMax = sdServer;
        }

        return sdServer;
    }

    return 0;       //Still connecting!!  Try back later...
}

//Read the network, handle any incoming data
int netclient::run()
{
    int rv = 0;

    try {
        //RECEIVE DATA ON ALL INCOMING CONNECTIONS
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
