/******************************************************************************
 *
 * Purpose:  Various utility functions intended to be private to the library.
 * 
 ******************************************************************************
 * Copyright (c) 2009
 * PCI Geomatics, 50 West Wilmot Street, Richmond Hill, Ont, Canada
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "pcidsk_p.h"

using namespace PCIDSK;

/************************************************************************/
/*                             atouint64()                              */
/************************************************************************/

uint64 PCIDSK::atouint64( const char *str_value )

{
#if defined(__MSVCRT__) || defined(_MSC_VER)
    return (uint64) _atoi64( str_value );
#else
    return (uint64) atoll( str_value );
#endif
}

/************************************************************************/
/*                              atoint64()                              */
/************************************************************************/

int64 PCIDSK::atoint64( const char *str_value )

{
#if defined(__MSVCRT__) || defined(_MSC_VER)
    return (int64) _atoi64( str_value );
#else
    return (int64) atoll( str_value );
#endif
}

/************************************************************************/
/*                              SwapData()                              */
/************************************************************************/

void PCIDSK::SwapData( void *data, int size, int count )

{
    uint8 *data8 = (uint8 *) data;

    if( size == 2 )
    {
        uint8 t;

        for( ; count; count-- )
        {
            t = data8[0];
            data8[0] = data8[1];
            data8[1] = t;

            data8 += 2;
        }
    }
    else if( size == 1 )
        /* do nothing */; 
    else if( size == 4 )
    {
        uint8 t;

        for( ; count; count-- )
        {
            t = data8[0];
            data8[0] = data8[3];
            data8[3] = t;

            t = data8[1];
            data8[1] = data8[2];
            data8[2] = t;

            data8 += 4;
        }
    }
    else
        throw new PCIDSKException( "Unsupported data size in SwapData()" );
}

