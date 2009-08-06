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
 * @return an internal string representing the data type.  The string should
 * not be altered or freed.
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
 * @return the internal string for the segment type.  The string should
 * not be altered or freed.
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
