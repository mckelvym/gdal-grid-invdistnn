/******************************************************************************
 *
 * Purpose:  Various utility functions.
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
/*                            DataTypeSize()                            */
/************************************************************************/

int PCIDSK::DataTypeSize( eChanType chan_type )

{
    switch( chan_type )
    {
      case CHN_8U:
        return 1;
      case CHN_16S:
        return 2;
      case CHN_16U:
        return 2;
      case CHN_32R:
        return 4;
      default:
        return 0;
    }
}

/************************************************************************/
/*                            DataTypeName()                            */
/************************************************************************/

const char *PCIDSK::DataTypeName( eChanType chan_type )

{
    switch( chan_type )
    {
      case CHN_8U:
        return "8U";
      case CHN_16S:
        return "16S";
      case CHN_16U:
        return "16U";
      case CHN_32R:
        return "32R";
      default:
        return "UNK";
    }
}
/************************************************************************/
/*                          SegmentTypeName()                           */
/************************************************************************/

const char *PCIDSK::SegmentTypeName( eSegType type )

{
    switch( type )
    {
      case SEG_BIT:
        return "BIT";
      case SEG_VEC:
        return "VEC";
      case SEG_SIG:
        return "SIG";
      case SEG_TEX:
        return "TEX";
      case SEG_GEO:
        return "GEO";
      case SEG_ORB:
        return "ORB";
      case SEG_LUT:
        return "LUT";
      case SEG_PCT:
        return "PCT";
      case SEG_BLUT:
        return "BLUT";
      case SEG_BPCT:
        return "BPCT";
      case SEG_BIN:
        return "BIN";
      case SEG_ARR:
        return "ARR";
      case SEG_SYS:
        return "SYS";
      case SEG_GCPOLD:
        return "GCPOLD";
      case SEG_GCP2:
        return "GCP2";
      default:
        return "UNKNOWN";
    }
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

