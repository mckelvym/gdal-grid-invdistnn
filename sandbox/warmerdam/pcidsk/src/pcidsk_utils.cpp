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
