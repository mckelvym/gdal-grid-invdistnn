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
#include "pcidsk_config.h"
#include "pcidsk_types.h"
#include "pcidsk_exception.h"
#include "core/pcidsk_utils.h"

using namespace PCIDSK;

/************************************************************************/
/*                            DataTypeSize()                            */
/************************************************************************/

/**
 * Return size of data type.
 *
 * @param chan_type the channel type enumeration value.
 *
 * @return the size of the passed data type in bytes, or zero for unknown 
 * values.
 */

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

/**
 * Return name for the data type.
 *
 * The returned values are suitable for display to people, and matches
 * the portion of the name after the underscore (ie. "8U" for CHN_8U.
 *
 * @param chan_type the channel type enumeration value to be translated.
 *
 * @return a string representing the data type.
 */

std::string PCIDSK::DataTypeName( eChanType chan_type )

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

/**
 * Return name for segment type.
 *
 * Returns a short name for the segment type code passed in.  This is normally
 * the portion of the enumeration name that comes after the underscore - ie. 
 * "BIT" for SEG_BIT. 
 * 
 * @param type the segment type code.
 *
 * @return the string for the segment type.
 */

std::string PCIDSK::SegmentTypeName( eSegType type )

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
/*                         GetCurrentDateTime()                         */
/************************************************************************/

// format we want: "HH:MM DDMMMYYYY \0"

#include <time.h>
#include <sys/types.h>

void	PCIDSK::GetCurrentDateTime( char *out_time )

{
    time_t	    clock;
    char            ctime_out[25];

    time( &clock );
    strncpy( ctime_out, ctime(&clock), 24 ); // TODO: reentrance issue?

    // ctime() products: "Wed Jun 30 21:49:08 1993\n"

    ctime_out[24] = '\0';

    out_time[0] = ctime_out[11];
    out_time[1] = ctime_out[12];
    out_time[2] = ':';
    out_time[3] = ctime_out[14];
    out_time[4] = ctime_out[15];
    out_time[5] = ' ';
    out_time[6] = ctime_out[8];
    out_time[7] = ctime_out[9];
    out_time[8] = ctime_out[4];
    out_time[9] = ctime_out[5];
    out_time[10] = ctime_out[6];
    out_time[11] = ctime_out[20];
    out_time[12] = ctime_out[21];
    out_time[13] = ctime_out[22];
    out_time[14] = ctime_out[23];
    out_time[15] = ' ';
    out_time[16] = '\0';
}

/************************************************************************/
/*                              UCaseStr()                              */
/*                                                                      */
/*      Force a string into upper case "in place".                      */
/************************************************************************/

std::string &PCIDSK::UCaseStr( std::string &target )

{
    for( unsigned int i = 0; i < target.size(); i++ )
    {
        if( islower(target[i]) )
            target[i] = toupper(target[i]);
    }
    
    return target;
}

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
        ThrowPCIDSKException( "Unsupported data size in SwapData()" );
}

