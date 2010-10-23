
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
// Read out data from packet, increment length
//

//Read arbitrary class in network byte order
template <class T> size_t netpacket::read( T& val)
{
    static unsigned long x = 0xFFFF0000;
    const size_t size = sizeof(T);

    //Check if byte order needs to be reversed
    if ( ntohl(x) == x ) {
        //Copy the data with no change
        memcpy( &val, &(data[length]), size); //copy all bytes
    } else {
        //Reverse the byte order, then copy
        for (x = 0; x < size; x++)
        {
            ((unsigned char *)&val)[x] = data[length + size - x - 1];
        }
    }
    length += size;
    return length;
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
    
    //length was already incremented by ::read<T>
    
    return length;
}

//1 bit integer (uses 8 bits)
size_t netpacket::read(bool &val)
{
    val = (bool)data[length++];
    
    return length;
}

//8 bit integer
size_t netpacket::read(unsigned char &val)
{
    val = data[length++];
    
    return length;
}

//8 bit character
size_t netpacket::read(char &val)
{
    val = (char)data[length++];
    
    return length;
}

//16 bit integer
size_t netpacket::read(unsigned short &val)
{
    val = ntohs(*(unsigned short*)&data[length]);
    length += sizeof(unsigned short);

    return length;
}

//16 bit integer
size_t netpacket::read(short &val)
{
    val = ntohs(*(short*)&data[length]);
    length += sizeof(short);

    return length;
}

//32 bit integer
size_t netpacket::read(unsigned long &val)
{
    val = ntohl(*(unsigned long*)&data[length]);
    length += sizeof(unsigned long);

    return length;
}

//32 bit integer
size_t netpacket::read(long &val)
{
    val = ntohl(*(long*)&data[length]);
    length += sizeof(long);
    
    return length;
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
    memcpy( val, &data[length], size);
    length += size;
    
    return length;
}

//byte array
size_t netpacket::read(unsigned char *val, size_t size)
{
    memcpy( val, &data[length], size);
    length += size;

    return length;
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
                memcpy( &(data[length]), &val, size); //copy all bytes
        } else {
                //Reverse the byte order, then copy
                unsigned char *r_array = new unsigned char[ size ];
                for (x = 0; x < size; x++)
                {
                        //r_array[x] = ((unsigned char *)&val)[size - x - 1];
                        data[length + x] = ((unsigned char *)&val)[size - x - 1];
                }
                //memcpy( &(data[length]), r_array, size); //copy all bytes
                delete r_array;
        }
        length += size;
        return length;
}

//Append array of any class (reverse each element)
template <class T> size_t netpacket::append( const T *val_array, size_t size)
{
    size_t n;
    for (n = 0; n < size; n++)
    {
        netpacket::append<T>( val_array[n] );
    }
    
    //length was incremented by netpacket::append<T>()
    
    return length;
}



//1 bit integer (uses 8 bits)
size_t netpacket::append (bool val)
{
        data[length] = (unsigned char)val;
        length++;
        
        return length;
}

//8 bit integer
size_t netpacket::append (unsigned char val)
{
        data[length] = val;
        length++;
        
        return length;
}

//8 bit character                     
size_t netpacket::append (char val)
{
        data[length] = (unsigned char)val;
        
        return length;
}
                              
//16 bit integer
size_t netpacket::append (unsigned short val)
{        
        val = htons(val);
        memcpy( data, &val, 2); //copy 2 bytes
        length += 2;
        
        return length;
}
                    
//16 bit integer
size_t netpacket::append (short val)
{
        val = htons(val);
        memcpy( data, &val, 2); //copy 2 bytes
        length += 2;
        
        return length;
}

//32 bit integer
size_t netpacket::append (unsigned long val)
{
        val = htonl(val);
        memcpy( data, &val, sizeof(unsigned long)); //copy 4 bytes
        length += sizeof(unsigned long);
        
        return length;
}

//32 bit integer
size_t netpacket::append (long val)
{
        val = htonl(val);
        memcpy( data, &val, sizeof(long)); //copy 4 bytes
        length += sizeof(long);
        
        return length;
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
  
    memcpy( &(data[length]), val, size); //copy all bytes
    length += size;
    return length;
}

//Byte array.  Don't reverse it :)
size_t netpacket::append (const unsigned char *val, size_t size)
{
    memcpy( &(data[length]), val, size); //copy all bytes
    length += size;
    return length;        
}

//TODO: wchar_t array
