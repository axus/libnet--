// Class for reading generic byte array, sent from the network or received from the network
//    Use an object for one or the other, not both!

#ifndef NETPACKET_H
#define NETPACKET_H

#include <cstdlib>

//Class for generic byte array to be sent
class netpacket {
    protected:
        size_t maxsize, position;
        unsigned char* data;
        bool delete_data;
    public:
        static const size_t DEFAULT_PACKET_SIZE = 1024;
        
        typedef size_t (*netPktCB)( netpacket* pkt, void *cb_data); //message processing callback type

	   //Default packet allocates 1024 bytes, 0 position.  append() more to increase position.
        netpacket( ):
                maxsize(DEFAULT_PACKET_SIZE), position(0), data(NULL), delete_data(true), ID(0)
                    { data = new unsigned char[DEFAULT_PACKET_SIZE];};
                    
        //Packet allocates *size* bytes, 0 position.  append() more to increase position.
        netpacket( size_t size):
                maxsize(size), position(0), data(0), delete_data(true), ID(0)
                    { data = new unsigned char[size];};

        //Packet points to pre-made buffer, position == maxsize.  Only good for sending or copying.
        netpacket( size_t size, unsigned char* buf):
                maxsize(size), position(size), data(buf), delete_data(false), ID(0)
                    {;};
        
        //Packet points to pre-made buffer, position == ptr.  You can append up to maxsize.
        netpacket( size_t size, unsigned char* buf, size_t ptr):
                maxsize(size), position(ptr), data(buf), delete_data(false), ID(0)
                    {;};
        
    //Destructors
        virtual ~netpacket();

    //Info
        size_t ID;   //Unique identifier for this packet... if you bother to set it!
        
        size_t get_position() const { return position; }; //Return used byte count
        size_t get_maxsize() const { return maxsize; }; //Return max byte count
        const unsigned char* get_ptr() const { return data; }; //Return pointer to entire packet data
    
    //Read from packet
        template <class T> size_t read(T&);                   //Read any class
        template <class T> size_t read ( const T *val_array, size_t size);    //Read array of any class
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
        size_t read (char *val, size_t size);          //Character string
        size_t read (unsigned char *val, size_t size); //byte array

    //Append to packet
        template <class T> size_t append(T val);               //Append any class
        template <class T> size_t append( const T *val_array, size_t size);    //Append array of any class
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
        void reset() { position = 0;};
};


#endif
