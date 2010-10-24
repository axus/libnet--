//Test program for "netserver" class
//  Wait for incoming clients, send HTML to any message

#include <netserver.h>
#include <cstdio>
#include <string>

using namespace std;

//Sleep function
#ifdef _WIN32
    #include <windows.h>
#else
    #include <cunistd>
    #define Sleep(n) usleep( n * 1000 )
#endif

//Constants
const size_t sleepytime = 500;      //0.5s
const size_t timeouttime = 60000;   //60s

//Callbacks
size_t send_response( netpacket* pkt, void *cb_data);
size_t print_disconnect( int c, void *cb_data);
size_t add_disconnectCB( int c, void *cb_data);

//Types
typedef struct  {
    netserver *server;
    char *msg;
    size_t length;
} serverResponse;

typedef struct {
    netserver *server;
    netbase::connectionFP cb;
} serverCallback;

//MAIN
int main (int argc, char *argv[])
{
  
    const string server = "localhost";
    const short lport = 80;
    const string http_response = "HTTP/1.1 200 OK\
Date: Sat, 23 Oct 2010 00:09:27 GMT\
Expires: -1\
Cache-Control: private, max-age=0\
Content-Type: text/html; charset=ISO-8859-1\
Server: gws\
X-XSS-Protection: 1; mode=block\
Transfer-Encoding: chunked\
\r\n\
81\r\n\
<!doctype html><html><head><title>NOPAGE</title></head><body>NOPAGE</body></html>";

    //Create server for up to 10 clients
    netserver Server(10);

    //Create buffer with HTTP response
    char buffer[netbase::NETMM_MAX_RECV_SIZE];
    strncpy( buffer, http_response.c_str(), http_response.length() + 1);
    
    //Set incoming packet callback
    serverResponse response_data = { &Server , buffer, http_response.length() + 1};
    Server.setPktCB( send_response, &response_data);

    //Set client disconnect callback
    serverCallback connect_data = { &Server, print_disconnect};
    Server.setConnectCB( add_disconnectCB, &connect_data);

    //Start listening on lport
    Server.openPort(lport);
    
    //Loop until timeout passed with no incoming connections
    size_t passedtime = 0;
    int rv = 0;
    while (passedtime < timeouttime) {
        Sleep(sleepytime);
        rv = Server.run();
        if (rv == 0) {
            passedtime += sleepytime;
            cout << "." << flush;
        }
    }
    
    return rv;
}

//Incoming packet callback
size_t send_response( netpacket* pkt, void *cb_data)
{
    //Get packet info
    int connection = pkt->ID;
    size_t result;

    //Get response info from cb_data
    serverResponse *response = (serverResponse*)cb_data;
    char *msg = response->msg;

    //Create packet with response info
    netpacket http_response_pkt( response->length, (unsigned char *)msg);
    
    //Send response packet on connection where we received a packet
    result = response->server->sendPacket(connection, http_response_pkt);
    cout << "s" << flush;
    
    //Return value of sendPacket
    return result;
}

// Connection callback
size_t add_disconnectCB( int c, void *cb_data)
{
    cout << "Connect on #" << c << endl;
    serverCallback *data = (serverCallback*)cb_data;
    
    //Set connection specific disconnect on server
    data->server->setDisconnectCB( c, data->cb, data->server);
    
    return 0;
}


//Disconnection callback
size_t print_disconnect( int c, void *cb_data)
{
    cout << "Disconnect on #" << c << endl;
    return 0;
}

