// Class for reading generic byte array, sent from the network or received from the network
//    Use an object for one or the other, not both!

#ifndef NETPACKET_H
#define NETPACKET_H

#include <cstdlib>

//Class for generic byte array to be sent
class netpacket {
    private:
        size_t maxsize;
        unsigned char* data;
        size_t pointer;
        bool delete_data;
    public:
        static const size_t DEFAULT_PACKET_SIZE = 1024;
        typedef size_t (*netMsgCB)( netpacket& msg, void *cb_data); //message processing callback type

	//Constuctors: you must choose how much memory to allocate when you create the packet :(
        netpacket( ):
                maxsize(DEFAULT_PACKET_SIZE), data(0), pointer(0), delete_data(true) { data = new unsigned char[DEFAULT_PACKET_SIZE];};
        netpacket( size_t size):
                maxsize(size), data(0), pointer(0), delete_data(true) { data = new unsigned char[size];};
        netpacket( size_t size, unsigned char* buf):
                maxsize(size), data(buf), pointer(0), delete_data(false) {;};
        netpacket( size_t size, unsigned char* buf, size_t ptr):
                maxsize(size), data(buf), pointer(ptr), delete_data(false) {;};
        
    //Destructors
        virtual ~netpacket();

    //Info
        size_t get_length() const { return pointer; }; //Return used byte count
        size_t get_maxsize() const { return maxsize; }; //Return max byte count
        const unsigned char* get_ptr() { return data; }; //Return pointer to packet data
    
    //Read from packet
        template <class T> size_t read(T&);                           //Read arbitrary class
        size_t read (bool& val);                              //1 bit integer (uses 8 bits)
        size_t read (unsigned char& val);                     //8 bit integer
        size_t read (char& val);                              //8 bit character
        size_t read (unsigned short& val);                    //16 bit integer
        size_t read (short& val);                             //16 bit integer
        size_t read (unsigned long& val);                     //32 bit integer
        size_t read (long& val);                              //32 bit integer
        size_t read (long long& val);                         //64 bit integer
        size_t read (float& val);                             //32 bit IEEE float
        size_t read (double& val);                            //64 bit IEEE float
        size_t read (const char *val, size_t size);          //Character string
        size_t read (const unsigned char *val, size_t size); //byte array
    
    //Append to packet
        template <class T> size_t append(T val);               //Append arbitrary class
        size_t append (bool val);                              //1 bit integer (uses 8 bits)
        size_t append (unsigned char val);                     //8 bit integer
        size_t append (char val);                              //8 bit character
        size_t append (unsigned short val);                    //16 bit integer
        size_t append (short val);                             //16 bit integer
        size_t append (unsigned long val);                     //32 bit integer
        size_t append (long val);                              //32 bit integer
        size_t append (long long val);                         //64 bit integer
        size_t append (float val);                             //32 bit IEEE float
        size_t append (double val);                            //64 bit IEEE float
        size_t append (const char *val, size_t size);          //Character string
        size_t append (const unsigned char *val, size_t size); //byte array

    //Reset packet (to reuse the packet without resizing)
        void reset() { pointer = 0;};
};


#endif
