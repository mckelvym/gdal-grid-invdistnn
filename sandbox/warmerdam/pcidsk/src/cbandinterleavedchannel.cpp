/******************************************************************************
 *
 * Purpose:  Implementation of the CBandInterleavedChannel class.
 *
 * This class is used to implement band interleaved channels within a 
 * PCIDSK file (which are always packed, and FILE interleaved data from
 * external raw files which may not be packed. 
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
/*                      CBandInterleavedChannel()                       */
/************************************************************************/

CBandInterleavedChannel::CBandInterleavedChannel( PCIDSKBuffer &image_header, 
                                                  PCIDSKBuffer &file_header,
                                                  int channelnum,
                                                  CPCIDSKFile *file,
                                                  uint64 image_offset,
                                                  eChanType pixel_type )
        : CPCIDSKChannel( image_header, file, pixel_type )

{
    io_handle_p = NULL;
    io_mutex_p = NULL;

/* -------------------------------------------------------------------- */
/*      Establish the data layout.                                      */
/* -------------------------------------------------------------------- */
    byte_order = image_header.buffer[201];

    if( strncmp(file_header.Get(360,8),"FILE",4) == 0 )
    {
        start_byte = atouint64(image_header.Get( 168, 16 ));
        pixel_offset = atouint64(image_header.Get( 184, 8 ));
        line_offset = atouint64(image_header.Get( 192, 8 ));
    }
    else
    {
        start_byte = image_offset;
        pixel_offset = DataTypeSize(pixel_type);
        line_offset = pixel_offset * width;
    }

/* -------------------------------------------------------------------- */
/*      Establish the file we will be accessing.                        */
/* -------------------------------------------------------------------- */
    int i;

    strcpy( filename, image_header.Get( 64, 64 ) );
    for( i = 63; i >= 0 && filename[i] == ' '; i-- )
    {
        filename[i] = '\0';
    }

    if( strlen(filename) != 0 )
        throw new PCIDSKException( "External (FILE interleaved) channels not yet supported." );

    file->GetIODetails( &io_handle_p, &io_mutex_p );
}

/************************************************************************/
/*                      ~CBandInterleavedChannel()                      */
/************************************************************************/

CBandInterleavedChannel::~CBandInterleavedChannel()

{
}

/************************************************************************/
/*                             ReadBlock()                              */
/************************************************************************/

int CBandInterleavedChannel::ReadBlock( int block_index, void *buffer )

{
    uint64 offset = start_byte + line_offset * block_index;
    int    pixel_size = DataTypeSize( pixel_type );

    PCIDSKInterfaces *interfaces = file->GetInterfaces();

/* -------------------------------------------------------------------- */
/*      If the imagery is packed, we can read directly into the         */
/*      target buffer.                                                  */
/* -------------------------------------------------------------------- */
    if( pixel_size == (int) pixel_offset )
    {
        MutexHolder holder( *io_mutex_p );

        interfaces->io->Seek( *io_handle_p, offset, SEEK_SET );
        interfaces->io->Read( buffer, 1, pixel_size * width, *io_handle_p );
    }

/* -------------------------------------------------------------------- */
/*      Otherwise we allocate a working buffer that holds the whole     */
/*      line, read into that, and pick out our data of interest.        */
/* -------------------------------------------------------------------- */
    else
    {
        int  i;
        PCIDSKBuffer line_from_disk( (int) (pixel_offset*(width-1) + pixel_size) );
        char *this_pixel;

        MutexHolder holder( *io_mutex_p );
        
        interfaces->io->Seek( *io_handle_p, offset, SEEK_SET );
        interfaces->io->Read( buffer, 1, line_from_disk.buffer_size, 
                              *io_handle_p );

        for( i = 0, this_pixel = line_from_disk.buffer; i < width; i++ )
        {
            memcpy( ((char *) buffer) + pixel_size * i, 
                    this_pixel, pixel_size );
            this_pixel += pixel_size;
        }
    }

/* -------------------------------------------------------------------- */
/*      Do we need to byte swap the data?                               */
/* -------------------------------------------------------------------- */
    /* TODO */

    return 1;
}


/************************************************************************/
/*                             WriteBlock()                             */
/************************************************************************/

int CBandInterleavedChannel::WriteBlock( int block_index, void *buffer )

{
    throw new PCIDSKException( "WriteBlock not implemented yet." );
}

