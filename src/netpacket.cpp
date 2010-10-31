
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
        data = NULL;
    }
}

//Adjust position, for reusing packet
void netpacket::setp( size_t p) { position = p;}

//
// Read out data from packet, increment position
//

//Read arbitrary class in network byte order
template <class T> size_t netpacket::read( T& val)
{
    static uint32_t x = 0xFFFF0000;
    const size_t size = sizeof(T);

    //Check if byte order needs to be reversed
    if ( ntohl(x) == x ) {
        //Copy the data with no change
        memcpy( &val, data + position, size); //copy all bytes
    } else {
        //Reverse the byte order, then copy
        for (x = 0; x < size; x++)
        {
            ((uint8_t *)&val)[x] = data[position + size - x - 1];
        }
    }
    position += size;
    return position;
}

//Read Generic class array into val. 
template <class T> size_t netpacket::read ( const T *val_array, size_t size)
{
    //Hates NULLs
    if (val_array == NULL)
        return ~0;
    
    size_t n;
    for (n = 0; n < size; n++) {
        //Copy from the packet to nth element of val_array
        netpacket::read<T>( *(val_array + n) );
    }
    
    //position was already incremented by ::read<T>
    
    return position;
}

//1 bit integer (uses 8 bits)
size_t netpacket::read(bool &val)
{
    val = (bool)data[position++];
    
    return position;
}

//8 bit integer
size_t netpacket::read(uint8_t &val)
{
    val = data[position++];
    
    return position;
}

//8 bit character
size_t netpacket::read(char &val)
{
    val = (char)data[position++];
    
    return position;
}

//16 bit integer
size_t netpacket::read(uint16_t &val)
{
    val = ntohs(*(uint16_t*)(data + position));
    position += sizeof(uint16_t);

    return position;
}

//16 bit integer
size_t netpacket::read(int16_t &val)
{
    val = ntohs(*(int16_t*)(data + position));
    position += sizeof(int16_t);

    return position;
}

//32 bit integer
size_t netpacket::read(uint32_t &val)
{
    val = ntohl(*(uint32_t*)(data + position));
    position += sizeof(uint32_t);

    return position;
}

//32 bit integer
size_t netpacket::read(int32_t &val)
{
    val = ntohl(*(int32_t*)(data + position));
    position += sizeof(int32_t);
    
    return position;
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
    memcpy( val, data + position, size);
    position += size;
    
    return position;
}

//byte array
size_t netpacket::read(uint8_t *val, size_t size)
{
    memcpy( val, data + position, size);
    position += size;

    return position;
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
                memcpy( data + position, &val, size); //copy all bytes
        } else {
                //For each byte in val
                for (x = 0; x < size; x++)
                {
                    //Copy val bytes to data, in reverse order
                    data[position + x] = ((uint8_t *)&val)[size - x - 1];
                }
        }
        position += size;
        return position;
}

//Append array of any class (reverse each element)
template <class T> size_t netpacket::append( const T *val_array, size_t size)
{
    size_t n;
    for (n = 0; n < size; n++)
    {
        netpacket::append<T>( val_array[n] );
    }
    
    //position was incremented by netpacket::append<T>()
    position += size;
    return position;
}



//1 bit integer (uses 8 bits)
size_t netpacket::append (bool val)
{
        data[position] = (uint8_t)val;
        position++;
        
        return position;
}

//8 bit integer
size_t netpacket::append (uint8_t val)
{
        data[position] = val;
        position++;
        
        return position;
}

//8 bit character                     
size_t netpacket::append (char val)
{
        data[position] = (uint8_t)val;
        position++;
        
        return position;
}
                              
//16 bit integer
size_t netpacket::append (uint16_t val)
{        
        val = htons(val);
        memcpy( data + position, &val, 2); //copy 2 bytes
        position += 2;
        
        return position;
}
                    
//16 bit integer
size_t netpacket::append (int16_t val)
{
        val = htons(val);
        memcpy( data + position, &val, 2); //copy 2 bytes
        position += 2;
        
        return position;
}

//32 bit integer
size_t netpacket::append (uint32_t val)
{
        val = htonl(val);
        memcpy( data + position, &val, sizeof(uint32_t)); //copy 4 bytes
        position += sizeof(uint32_t);
        
        return position;
}

//32 bit integer
size_t netpacket::append (int32_t val)
{
        val = htonl(val);
        memcpy( data + position, &val, sizeof(int32_t)); //copy 4 bytes
        position += sizeof(int32_t);
        
        return position;
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
  
    memcpy( data + position, val, size); //copy all bytes
    position += size;
    return position;
}

//Byte array.  Don't reverse it :)
size_t netpacket::append (const uint8_t *val, size_t size)
{
    memcpy( data + position, val, size); //copy all bytes
    position += size;
    return position;        
}

//TODO: wchar_t array
