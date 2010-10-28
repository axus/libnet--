
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

//
// Read out data from packet, increment position
//

//Read arbitrary class in network byte order
template <class T> size_t netpacket::read( T& val)
{
    static unsigned long x = 0xFFFF0000;
    const size_t size = sizeof(T);

    //Check if byte order needs to be reversed
    if ( ntohl(x) == x ) {
        //Copy the data with no change
        memcpy( &val, data + position, size); //copy all bytes
    } else {
        //Reverse the byte order, then copy
        for (x = 0; x < size; x++)
        {
            ((unsigned char *)&val)[x] = data[position + size - x - 1];
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
size_t netpacket::read(unsigned char &val)
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
size_t netpacket::read(unsigned short &val)
{
    val = ntohs(*(unsigned short*)(data + position));
    position += sizeof(unsigned short);

    return position;
}

//16 bit integer
size_t netpacket::read(short &val)
{
    val = ntohs(*(short*)(data + position));
    position += sizeof(short);

    return position;
}

//32 bit integer
size_t netpacket::read(unsigned long &val)
{
    val = ntohl(*(unsigned long*)(data + position));
    position += sizeof(unsigned long);

    return position;
}

//32 bit integer
size_t netpacket::read(long &val)
{
    val = ntohl(*(long*)(data + position));
    position += sizeof(long);
    
    return position;
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
size_t netpacket::read(char *val, size_t size)
{
    memcpy( val, data + position, size);
    position += size;
    
    return position;
}

//byte array
size_t netpacket::read(unsigned char *val, size_t size)
{
    memcpy( val, data + position, size);
    position += size;

    return position;
}

//
//Append data to packet in "network byte order".  short, long, float, double.
//

//Append any class (reverse its bytes on x86)
template <class T> size_t netpacket::append(T val) {
        static unsigned long x = 0xFFFF0000;
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
                    data[position + x] = ((unsigned char *)&val)[size - x - 1];
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
        data[position] = (unsigned char)val;
        position++;
        
        return position;
}

//8 bit integer
size_t netpacket::append (unsigned char val)
{
        data[position] = val;
        position++;
        
        return position;
}

//8 bit character                     
size_t netpacket::append (char val)
{
        data[position] = (unsigned char)val;
        position++;
        
        return position;
}
                              
//16 bit integer
size_t netpacket::append (unsigned short val)
{        
        val = htons(val);
        memcpy( data + position, &val, 2); //copy 2 bytes
        position += 2;
        
        return position;
}
                    
//16 bit integer
size_t netpacket::append (short val)
{
        val = htons(val);
        memcpy( data + position, &val, 2); //copy 2 bytes
        position += 2;
        
        return position;
}

//32 bit integer
size_t netpacket::append (unsigned long val)
{
        val = htonl(val);
        memcpy( data + position, &val, sizeof(unsigned long)); //copy 4 bytes
        position += sizeof(unsigned long);
        
        return position;
}

//32 bit integer
size_t netpacket::append (long val)
{
        val = htonl(val);
        memcpy( data + position, &val, sizeof(long)); //copy 4 bytes
        position += sizeof(long);
        
        return position;
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
  
    memcpy( data + position, val, size); //copy all bytes
    position += size;
    return position;
}

//Byte array.  Don't reverse it :)
size_t netpacket::append (const unsigned char *val, size_t size)
{
    memcpy( data + position, val, size); //copy all bytes
    position += size;
    return position;        
}

//TODO: wchar_t array
