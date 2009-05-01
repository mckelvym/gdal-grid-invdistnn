/******************************************************************************
 *
 * Purpose:  Implementation of the SysVirtualFile class.
 *
 * This class is used to manage access to a single virtual file stored in
 * SysBData segments based on a block map stored in the SysBMap segment 
 * (and managed by SysBlockMap class). 
 *
 * The virtual files are allocated in 8K chunks (block_size) in segments.
 * To minimize IO requests, other overhead, we keep one such 8K block in
 * our working cache for the virtual file stream.  
 *
 * This class is primarily used by the CTiledChannel class for access to 
 * tiled images.
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
/*                           SysVirtualFile()                           */
/************************************************************************/

SysVirtualFile::SysVirtualFile( CPCIDSKFile *file, int start_block, 
                                uint64 image_length, 
                                PCIDSKBuffer &block_map_data )

{
    file_length = image_length;
    this->file = file;

    loaded_block = -1;

    int next_block = start_block;

    // perhaps we should defer all this work till the first request is made?
    while( next_block != -1 )
    {
        int offset = 512 + next_block * 28;

        block_segment.push_back( block_map_data.GetInt( offset+0, 4 ) );
        block_index.push_back( block_map_data.GetInt( offset+4, 8 ) );
        
        next_block = block_map_data.GetInt( offset + 20, 8 );
    }

    assert( block_index.size() * 8192 >= file_length );
}

/************************************************************************/
/*                          ~SysVirtualFile()                           */
/************************************************************************/

SysVirtualFile::~SysVirtualFile()

{
}


/************************************************************************/
/*                            WriteToFile()                             */
/************************************************************************/

void 
SysVirtualFile::WriteToFile( const void *buffer, uint64 offset, uint64 size )

{
    throw new PCIDSKException( 
        "SysVirtualFile::WriteToFile() not yet implemented." );
}

/************************************************************************/
/*                            ReadFromFile()                            */
/************************************************************************/

void SysVirtualFile::ReadFromFile( void *buffer, uint64 offset, uint64 size )

{
    uint64 buffer_offset = 0;

    while( buffer_offset < size )
    {
        int request_block = (int) ((offset + buffer_offset) / block_size);
        int offset_in_block = (int) ((offset + buffer_offset) % block_size);
        int amount_to_copy;

        LoadBlock( request_block );

        amount_to_copy = block_size - offset_in_block;
        if( amount_to_copy > (int) (size - buffer_offset) )
            amount_to_copy = size - buffer_offset;

        memcpy( ((uint8 *) buffer) + buffer_offset, 
                block_data + offset_in_block, amount_to_copy );

        buffer_offset += amount_to_copy;
    }
}

/************************************************************************/
/*                             LoadBlock()                              */
/************************************************************************/

void SysVirtualFile::LoadBlock( int requested_block )

{
    if( requested_block < 0 || requested_block >= (int) block_index.size() )
        throw new PCIDSKException( "SysVirtualFile::LoadBlock(%d) - block out of range.",
                                   requested_block );

    if( requested_block == loaded_block )
        return;

    PCIDSKSegment *data_seg_obj = 
        file->GetSegment( block_segment[requested_block] );

    data_seg_obj->ReadFromFile( block_data, 
                                block_size * block_index[requested_block],
                                block_size );

    loaded_block = requested_block;
}

