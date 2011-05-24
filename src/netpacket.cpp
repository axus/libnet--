
#include "netpacket.h"
#include <winsock2.h>
#include <cstring>

//
//  netpacket function implementations
//
using net__::netpacket;

//Constructors are defined in header

//Destructor
netpacket::~netpacket() {
    if (delete_data) {
        delete data;
        data = NULL;
    }
}

//Get version of libnet--
size_t netpacket::getVersion()
{
    return NETPACKET_VERSION;
}

//Adjust position, for reusing packet
void netpacket::set_read( size_t p) { pos_read = p;}
void netpacket::set_write( size_t p) { pos_write = p;}

//
// Read out data from packet, increment pos_read
//

//Read arbitrary class in network byte order
template <class T> size_t netpacket::read( T& val)
{
    static uint32_t x = 0xFFFF0000;
    const size_t size = sizeof(T);

    //Check if byte order needs to be reversed
    if ( ntohl(x) == x ) {
        //Copy the data with no change
        memcpy( &val, data + pos_read, size); //copy all bytes
    } else {
        //Reverse the byte order, then copy
        for (x = 0; x < size; x++)
        {
            ((uint8_t *)&val)[x] = data[pos_read + size - x - 1];
        }
    }
    pos_read += size;
    return pos_read;
}


//Read Generic class array into val. 
template <class T> size_t netpacket::read ( T* val_array, size_t size)
{
    //Hates NULLs
    if (val_array == NULL)
        return ~0;
    
    size_t n;
    for (n = 0; n < size; n++) {
        //Copy from the packet to nth element of val_array
        netpacket::read<T>( *(val_array + n) );
    }
    
    //pos_read was already incremented by ::read<T>
    
    return pos_read;
}

//1 bit integer (uses 8 bits)
size_t netpacket::read(bool &val)
{
    val = (data[pos_read++] != 0);
    
    return pos_read;
}

//8 bit integer
size_t netpacket::read(uint8_t &val)
{
    val = data[pos_read++];
    
    return pos_read;
}

//8 bit signed byte
size_t netpacket::read(int8_t &val)
{
    val = data[pos_read++];
    
    return pos_read;
}

//8 bit character
size_t netpacket::read(char &val)
{
    val = (char)data[pos_read++];
    
    return pos_read;
}

//16 bit integer
size_t netpacket::read(uint16_t &val)
{
    val = ntohs(*(uint16_t*)(data + pos_read));
    pos_read += sizeof(uint16_t);

    return pos_read;
}

//16 bit integer
size_t netpacket::read(int16_t &val)
{
    val = ntohs(*(int16_t*)(data + pos_read));
    pos_read += sizeof(int16_t);

    return pos_read;
}

//32 bit integer
size_t netpacket::read(uint32_t &val)
{
    val = ntohl(*(uint32_t*)(data + pos_read));
    pos_read += sizeof(uint32_t);

    return pos_read;
}

//32 bit integer
size_t netpacket::read(int32_t &val)
{
    val = ntohl(*(int32_t*)(data + pos_read));
    pos_read += sizeof(int32_t);
    
    return pos_read;
}

//64 bit integer
size_t netpacket::read(int64_t &val)
{
    return read<int64_t>( val );
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
size_t netpacket::read(char *val, size_t size)
{
    memcpy( val, data + pos_read, size);
    pos_read += size;
    
    return pos_read;
}

//byte array
size_t netpacket::read(uint8_t *val, size_t size)
{
    memcpy( val, data + pos_read, size);
    pos_read += size;

    return pos_read;
}

//short array.. use network to host conversion
size_t netpacket::read (uint16_t *val, size_t count)
{
    size_t index;
    for (index = 0; index < count; index++, pos_read += sizeof(uint16_t)) {
        val[index] = ntohs(*(int16_t*)(data + pos_read));
    }
    //memcpy( val, data + pos_read, count*sizeof(uint16_t));
    //pos_read += count*sizeof(uint16_t);

    return pos_read;
}

//
//Append data to packet in "network byte order".  int16_t, int32_t, float, double.
//

//Append any class (reverse its bytes on x86)
template <class T> size_t netpacket::append(T val) {
        static uint32_t x = 0xFFFF0000;
        const size_t size = sizeof(T);

        //Check if byte order needs to be reversed
        if ( htonl(x) == x ) {
                //Copy the data with no change
                memcpy( data + pos_write, &val, size); //copy all bytes
        } else {
                //For each byte in val
                for (x = 0; x < size; x++)
                {
                    //Copy val bytes to data, in reverse order
                    data[pos_write + x] = ((uint8_t *)&val)[size - x - 1];
                }
        }
        pos_write += size;
        return pos_write;
}

//Append array of any class (reverse each element)
template <class T> size_t netpacket::append( const T *val_array, size_t size)
{
    size_t n;
    for (n = 0; n < size; n++)
    {
        netpacket::append<T>( val_array[n] );
    }
    
    //pos_write was incremented by netpacket::append<T>()
    pos_write += size;
    return pos_write;
}



//1 bit integer (uses 8 bits)
size_t netpacket::append (bool val)
{
        data[pos_write] = (uint8_t)val;
        pos_write++;
        
        return pos_write;
}

//8 bit integer
size_t netpacket::append (uint8_t val)
{
        data[pos_write] = val;
        pos_write++;
        
        return pos_write;
}

//8 bit character                     
size_t netpacket::append (char val)
{
        data[pos_write] = (uint8_t)val;
        pos_write++;
        
        return pos_write;
}
                              
//16 bit integer
size_t netpacket::append (uint16_t val)
{        
        val = htons(val);
        memcpy( data + pos_write, &val, 2); //copy 2 bytes
        pos_write += 2;
        
        return pos_write;
}
                    
//16 bit integer
size_t netpacket::append (int16_t val)
{
        val = htons(val);
        memcpy( data + pos_write, &val, 2); //copy 2 bytes
        pos_write += 2;
        
        return pos_write;
}

//32 bit integer
size_t netpacket::append (uint32_t val)
{
        val = htonl(val);
        memcpy( data + pos_write, &val, sizeof(uint32_t)); //copy 4 bytes
        pos_write += sizeof(uint32_t);
        
        return pos_write;
}

//32 bit integer
size_t netpacket::append (int32_t val)
{
        val = htonl(val);
        memcpy( data + pos_write, &val, sizeof(int32_t)); //copy 4 bytes
        pos_write += sizeof(int32_t);
        
        return pos_write;
}

//64 bit integer
size_t netpacket::append (int64_t val)
{
        return append<int64_t>(val);
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
  
    memcpy( data + pos_write, val, size); //copy all bytes
    pos_write += size;
    return pos_write;
}

//Byte array.  Don't reverse it :)
size_t netpacket::append (const uint8_t *val, size_t size)
{
    memcpy( data + pos_write, val, size); //copy all bytes
    pos_write += size;
    return pos_write;
}

//Short array.  Use htons
size_t netpacket::append (const uint16_t *val, size_t count)
{
    size_t index;
    for (index = 0; index < count; index++, pos_write += sizeof(uint16_t)) {
        *(int16_t*)(data + pos_write) = htons(val[index]);
    }
    return count;
}

//TODO: wchar_t array
