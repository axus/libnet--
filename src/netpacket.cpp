
#include "netpacket.h"
#include <winsock2.h>
#include <cstring>

//
//  netpacket function implementations
//

//Constructors are defined in header

//Destructor
netpacket::~netpacket() {
    if (delete_data) {
        delete data;
    }
}

//
// Read out data from packet, increment pointer
//

//Read arbitrary class in network byte order
template <class T> size_t netpacket::read( T& val)
{
    static unsigned long x = 0xFFFF0000;
    const size_t size = sizeof(T);

    //Check if byte order needs to be reversed
    if ( ntohl(x) == x ) {
        //Copy the data with no change
        memcpy( &val, &(data[pointer]), size); //copy all bytes
    } else {
        //Reverse the byte order, then copy
        for (x = 0; x < size; x++)
        {
            ((unsigned char *)&val)[x] = data[pointer + size - x - 1];
        }
    }
    pointer += size;
    return pointer;
}

//1 bit integer (uses 8 bits)
size_t netpacket::read(bool &val)
{
    val = (bool)data[pointer++];
    
    return pointer;
}

//8 bit integer
size_t netpacket::read(unsigned char &val)
{
    val = data[pointer++];
    
    return pointer;
}

//8 bit character
size_t netpacket::read(char &val)
{
    val = (char)data[pointer++];
    
    return pointer;
}

//16 bit integer
size_t netpacket::read(unsigned short &val)
{
    val = ntohs(*(unsigned short*)&data[pointer]);
    pointer += sizeof(unsigned short);

    return pointer;
}

//16 bit integer
size_t netpacket::read(short &val)
{
    val = ntohs(*(short*)&data[pointer]);
    pointer += sizeof(short);

    return pointer;
}

//32 bit integer
size_t netpacket::read(unsigned long &val)
{
    val = ntohl(*(unsigned long*)&data[pointer]);
    pointer += sizeof(unsigned long);

    return pointer;
}

//32 bit integer
size_t netpacket::read(long &val)
{
    val = ntohl(*(long*)&data[pointer]);
    pointer += sizeof(long);
    
    return pointer;
}

//64 bit integer
size_t netpacket::read(long long &val)
{
    return read<long long>( val );
}

//32 bit IEEE float
size_t netpacket::read(float &val)
{
    return read<float>( val );
}

//64 bit IEEE float
size_t netpacket::read(double &val)
{
    return read<double>( val);
}

//Character string
size_t netpacket::read(const char *val, size_t size)
{
    memcpy( val, &data[position], size);
    position += size;
    
    return position;
}

//byte array
size_t netpacket::read(const unsigned char *val, size_t size)
{
    memcpy( val, &data[position], size);
    position += size;

    return position;
}

//
//Append data to packet in "network byte order".  short, long, float, double.
//
template <class T> size_t netpacket::append(T val) {
        static unsigned long x = 0xFFFF0000;
        const size_t size = sizeof(T);

        //Check if byte order needs to be reversed
        if ( htonl(x) == x ) {
                //Copy the data with no change
                memcpy( &(data[pointer]), &val, size); //copy all bytes
        } else {
                //Reverse the byte order, then copy
                unsigned char *r_array = new unsigned char[ size ];
                for (x = 0; x < size; x++)
                {
                        //r_array[x] = ((unsigned char *)&val)[size - x - 1];
                        data[pointer + x] = ((unsigned char *)&val)[size - x - 1];
                }
                //memcpy( &(data[pointer]), r_array, size); //copy all bytes
                delete r_array;
        }
        pointer += size;
        return pointer;
}


//1 bit integer (uses 8 bits)
size_t netpacket::append (bool val)
{
        data[pointer] = (unsigned char)val;
        pointer += 1;
        
        return pointer;
}

//8 bit integer
size_t netpacket::append (unsigned char val)
{
        data[pointer] = val;
        pointer += 1;
        
        return pointer;
}

//8 bit character                     
size_t netpacket::append (char val)
{
        data[pointer] = (unsigned char)val;
        
        return pointer;
}
                              
//16 bit integer
size_t netpacket::append (unsigned short val)
{        
        val = htons(val);
        memcpy( data, &val, 2); //copy 2 bytes
        pointer += 2;
        
        return pointer;
}
                    
//16 bit integer
size_t netpacket::append (short val)
{
        val = htons(val);
        memcpy( data, &val, 2); //copy 2 bytes
        pointer += 2;
        
        return pointer;
}

//32 bit integer
size_t netpacket::append (unsigned long val)
{
        val = htonl(val);
        memcpy( data, &val, sizeof(unsigned long)); //copy 4 bytes
        pointer += sizeof(unsigned long);
        
        return pointer;
}

//32 bit integer
size_t netpacket::append (long val)
{
        val = htonl(val);
        memcpy( data, &val, sizeof(long)); //copy 4 bytes
        pointer += sizeof(long);
        
        return pointer;
}

//64 bit integer
size_t netpacket::append (long long val)
{
        return append<long long>(val);
}

//32 bit IEEE float
size_t netpacket::append (float val)
{
        return append<float>(val);
}

//64 bit IEEE float
size_t netpacket::append (double val)
{
        return append<double>(val);
}

//Character array.  Don't reverse it :)
size_t netpacket::append (const char *val, const size_t size)
{
  
    memcpy( &(data[pointer]), val, size); //copy all bytes
    pointer += size;
    return pointer;
}

//Byte array.  Don't reverse it :)
size_t netpacket::append (const unsigned char *val, size_t size)
{
    memcpy( &(data[pointer]), val, size); //copy all bytes
    pointer += size;
    return pointer;        
}

//TODO: wchar_t array
