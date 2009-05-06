/******************************************************************************
 *
 * Purpose:  Implementation of the CPixelInterleavedChannel class.
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
#include <assert.h>

using namespace PCIDSK;

/************************************************************************/
/*                      CPixelInterleavedChannel()                      */
/************************************************************************/

CPixelInterleavedChannel::CPixelInterleavedChannel( PCIDSKBuffer &image_header, 
                                                    PCIDSKBuffer &file_header,
                                                    int channelnum,
                                                    CPCIDSKFile *file,
                                                    int image_offset,
                                                    eChanType pixel_type )
        : CPCIDSKChannel( image_header, file, pixel_type, channelnum )

{
    this->image_offset = image_offset;
}

/************************************************************************/
/*                     ~CPixelInterleavedChannel()                      */
/************************************************************************/

CPixelInterleavedChannel::~CPixelInterleavedChannel()

{
}

/************************************************************************/
/*                             ReadBlock()                              */
/************************************************************************/

int CPixelInterleavedChannel::ReadBlock( int block_index, void *buffer )

{
    int pixel_group = file->GetPixelGroupSize();
    int pixel_size = DataTypeSize(GetType());

/* -------------------------------------------------------------------- */
/*      Read and lock the scanline.                                     */
/* -------------------------------------------------------------------- */
    uint8 *pixel_buffer = (uint8 *) file->ReadAndLockBlock( block_index );

/* -------------------------------------------------------------------- */
/*      Copy the data into our callers buffer.  Try to do this          */
/*      reasonably efficiently.  We might consider adding faster        */
/*      cases for 16/32bit data that is word aligned.                   */
/* -------------------------------------------------------------------- */
    if( pixel_size == pixel_group )
        memcpy( buffer, pixel_buffer, pixel_size * width );
    else
    {
        int i;
        uint8  *src = ((uint8 *)pixel_buffer) + image_offset;
        uint8  *dst = (uint8 *) buffer;

        if( pixel_size == 1 )
        {
            for( i = width; i != 0; i-- )
            {
                *dst = *src;
                dst++;
                src += pixel_group;
            }
        }
        else if( pixel_size == 2 )
        {
            for( i = width; i != 0; i-- )
            {
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                src += pixel_group-2;
            }
        }
        else if( pixel_size == 4 )
        {
            for( i = width; i != 0; i-- )
            {
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                src += pixel_group-4;
            }
        }
        else
            throw new PCIDSKException( "Unsupported pixel type..." );
    }
    
    file->UnlockBlock( 0 );

/* -------------------------------------------------------------------- */
/*      Do byte swapping if needed.                                     */
/* -------------------------------------------------------------------- */
    if( needs_swap )
        SwapData( buffer, pixel_size, width );

    return 0;
}


/************************************************************************/
/*                             WriteBlock()                             */
/************************************************************************/

int CPixelInterleavedChannel::WriteBlock( int block_index, void *buffer )

{
    throw new PCIDSKException( "WriteBlock not implemented yet." );
}

