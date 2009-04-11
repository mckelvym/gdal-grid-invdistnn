/******************************************************************************
 *
 * Purpose:  Implementation of the PCIDSKBuffer class.  This class is for
 *           convenient parsing and formatting of PCIDSK ASCII headers.
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
/*                            PCIDSKBuffer()                            */
/************************************************************************/

PCIDSKBuffer::PCIDSKBuffer( int size )

{
    buffer_size = size;
    buffer = (char *) malloc(size+1);

    if( buffer == NULL )
        throw PCIDSKException( "Out of memory allocating %d byte PCIDSKBuffer.",
                               size );

    buffer[size] = '\0';
}

/************************************************************************/
/*                           ~PCIDSKBuffer()                            */
/************************************************************************/

PCIDSKBuffer::~PCIDSKBuffer()

{
    free( buffer );
}

/************************************************************************/
/*                                Get()                                 */
/************************************************************************/

const char *PCIDSKBuffer::Get( int offset, int size )

{
    if( offset + size > buffer_size )
        throw PCIDSKException( "Get() past end of PCIDSKBuffer." );

    work_field.assign( buffer + offset, size );

    return work_field.c_str();
}

/************************************************************************/
/*                                Put()                                 */
/************************************************************************/

void PCIDSKBuffer::Put( const char *value, int offset, int size )

{
    if( offset + size > buffer_size )
        throw PCIDSKException( "Put() past end of PCIDSKBuffer." );

    int v_size = strlen(value);
    if( v_size > size )
        v_size = size;

    if( v_size < size )
        memset( buffer + offset, ' ', size );

    memcpy( buffer + offset, value, v_size );
}

