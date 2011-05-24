// Class for reading generic byte array, sent from the network or received from
//      the network.   Use one object for read OR write, not both!

#ifndef NETPACKET_H
#define NETPACKET_H

#include <cstdlib>

//Compiler specific options
#ifdef _MSC_VER
    #include "ms_stdint.h"
    #define sock_t SOCKET
#else
    #include <stdint.h>
    #define sock_t int
#endif

//getVersion() should match this value!
#define NETPACKET_VERSION 0x0200

namespace net__ {

    //Class for generic byte array to be sent
    class netpacket {
        protected:
            size_t maxsize, pos_read, pos_write;
    
            uint8_t* data;
            bool delete_data;
            
        public:
            static const size_t DEFAULT_PACKET_SIZE = 1024;
            
            //netPktCB: packet callback function type
            typedef size_t (*netPktCB)( netpacket* pkt, void *cb_data);
            //  Packet callback functions must return the number of bytes
            //  read from the packet.  However, if the callback function
            //  disconnects from remote host, it should instead return 0.

    	   //Constructor: Allocates 1024 bytes, ready to read or write.
            netpacket( ):
                    maxsize(DEFAULT_PACKET_SIZE), pos_read(0), pos_write(0),
                    data(NULL), delete_data(true), ID(0)
            {
                data = new uint8_t[DEFAULT_PACKET_SIZE];
            };
                        
            //Constructor: Allocates *size* bytes, ready to read or write.
            netpacket( size_t size):
                    maxsize(size), pos_read(0), pos_write(0), data(0),
                    delete_data(true), ID(0)
            {
                data = new uint8_t[size];
            };
    
            //Constructor: Point to a pre-made buffer, ready to read.
            //             User should not write to packet
            netpacket( size_t size, uint8_t* buf):
                    maxsize(size), pos_read(0), pos_write(size), data(buf),
                    delete_data(false), ID(0)
            {
                ;
            };
            
            //Constructor: Points to pre-made buffer, ready to read.
            //              Writing starts at write_index.
            netpacket( size_t size, uint8_t* buf, size_t write_index):
                    maxsize(size), pos_read(0), pos_write(write_index),
                    data(buf), delete_data(false), ID(0)
            {
                ;
            };
            
        //Destructors
            virtual ~netpacket();
    
        //Info
            //Return library version
            static size_t getVersion();

            sock_t ID;   //Associate packet with socket
            
            //Return bytes written to packet
            size_t get_write() const { return pos_write; };
            
            //Return bytes read from packet
            size_t get_read() const { return pos_read; };
            
            //Return max byte count
            size_t get_maxsize() const { return maxsize; };
            
            //Return pointer to entire packet data
            const uint8_t* get_ptr() const { return data; };
        
        //Read from packet
            size_t read (bool& val);      //1 bit integer (uses 8 bits)
            size_t read (uint8_t& val);   //unsigned byte
            size_t read (int8_t& val);    //signed byte
            size_t read (char& val);      //8 bit character
            size_t read (uint16_t& val);  //16 bit integer
            size_t read (int16_t& val);   //16 bit integer
            size_t read (uint32_t& val);  //32 bit integer
            size_t read (int32_t& val);   //32 bit integer
            size_t read (int64_t& val);   //64 bit integer
            size_t read (float& val);     //32 bit IEEE float
            size_t read (double& val);    //64 bit IEEE float
            size_t read (char *val, size_t size);     //Character string
            size_t read (uint8_t *val, size_t size);  //byte array
            size_t read (uint16_t *val, size_t size); //short array
    
        //Append to packet
    
            size_t append (bool val);     //1 bit integer (uses 8 bits)
            size_t append (uint8_t val);  //8 bit integer
            size_t append (int8_t& val);  //signed byte
            size_t append (char val);     //8 bit character
            size_t append (uint16_t val); //16 bit integer
            size_t append (int16_t val);  //16 bit integer
            size_t append (uint32_t val); //32 bit integer
            size_t append (int32_t val);  //32 bit integer
            size_t append (int64_t val);  //64 bit integer
            size_t append (float val);    //32 bit IEEE float
            size_t append (double val);   //64 bit IEEE float
            size_t append (const char *val, size_t size);    //Char array
            size_t append (const uint8_t *val, size_t size); //byte array
            size_t append (const uint16_t *val, size_t size);//short array
    
        //Reset position (to reuse the packet without resizing)
            void set_read( size_t p=0);
            void set_write( size_t p=0);
            
        protected:
            //Template functions, can't make use of them outside the library :(
            template <class T> size_t read(T&);     //Read any class
            template <class T> size_t read(T* val_array,
                                    size_t size);    //Read array of any class
            template <class T> size_t append(T val); //Append any class
            template <class T> size_t append( const T *val_array,
                                    size_t size);    //Append array of any class
    };
}

#endif
