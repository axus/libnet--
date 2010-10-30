//Send HTTP request to Google, print the raw text
//  (test program for libnet--)
#include <netclient.h>
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
const size_t sleepytime = 500;
const size_t timeouttime = 5000;

//Callbacks
size_t print_pkt( netpacket* pkt, void *cb_data);

//MAIN
int main (int argc, char *argv[])
{
    //int result = 0;
    string server = "localhost";
    const short port = 80, lport = 13371;
    string http_request("GET / HTTP/1.1\r\n");

    //Command line option
    if (argc > 1) {
        server = argv[1];
    }
    
    //Complete http_request with Host
    http_request.append( "Host: ");
    http_request.append( server );
    http_request.append( "\r\n\r\n");

    //Create client
    netclient Client(2);
    
    //Add callback for all received packets
    //Client.setPktCB( print_pkt, NULL);

    //Create http GET packet        
    unsigned char buffer[netbase::NETMM_MAX_RECV_SIZE];
    size_t index=0;
    strncpy((char*)(buffer + index), http_request.c_str(), http_request.length());
    netpacket http_get_pkt( http_request.length(), buffer + index);

    //Connect to server on port 80
    int connection;
    connection = Client.doConnect(server.c_str(), port, lport);
    if (connection <= 0) {
        cout << "Connection error: " << Client.lastError << endl;
        return 1;
    }

    //Set connection callback
    Client.setConPktCB( connection, print_pkt, NULL);

    //Send HTTP request... to Google!
    Client.sendPacket( connection, http_get_pkt );
    
    //Loop until timeout
    size_t passedtime = 0;
    int rv = 0;
    while (passedtime < timeouttime) {
        Sleep(sleepytime);
        rv = Client.run();

        if (rv == SOCKET_ERROR) {
            cout << "Socket error: " << Client.lastError << endl;
            return 2;
        }
        else if (rv == 0) {
            //Nothing happened, increment passedtime
            passedtime += sleepytime;
        } else {
            //Packets were processed, restart timeout
            passedtime = 0;
        }
    }

    //Disconnect from server
    Client.disconnect(connection);

    return 0;
}

//Callback, return number of bytes printed from packet
size_t print_pkt( netpacket* pkt, void *cb_data)
{
    size_t result = pkt->get_maxsize();  //The bytes.  All of them.
    
    cout << (const char *)(pkt->get_ptr()) << endl;
    return result;
}
