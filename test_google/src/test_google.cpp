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
  
    const string server = "www.google.com";
    const short port = 80, lport = 13371;
    const string http_request("GET /index.html HTTP/1.1\r\n\r\n");

    //Create client
    netclient Client;
    
    //Add callback for all received packets
    Client.addCB( print_pkt, NULL);

    //Create http GET packet        
    unsigned char buffer[netbase::NET_MAX_RECV_SIZE];
    size_t index=0;
    strncpy((char*)(buffer + index), http_request.c_str(), http_request.length());
    netpacket http_get_pkt( http_request.length(), buffer + index);

    //Connect... to Google!
    int connection;
    connection = Client.doConnect(server.c_str(), port, lport);

    //Send HTTP request... to Google!
    Client.sendMessage( connection, http_get_pkt );
    
    //Loop until timeout
    size_t passedtime = 0;
    int rv = 0;
    while (passedtime < timeouttime) {
        Sleep(sleepytime);
        rv = Client.run();
        if (rv == 0) {
            passedtime += sleepytime;
        }
    }

    //Client.doDisconnect();
    
    return connection;
}

//Callback
size_t print_pkt( netpacket* pkt, void *cb_data)
{
    cout << (const char *)(pkt->get_ptr()) << endl;
    return 0;
}
