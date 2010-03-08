/******************************************************************************
 *
 * Purpose:  Implementation of the SysVirtualFile class.
 *
 * This class is used to manage access to a single virtual file stored in
 * SysBData segments based on a block map stored in the SysBMDir segment 
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
#include "pcidsk_config.h"
#include "pcidsk_types.h"
#include "pcidsk_buffer.h"
#include "pcidsk_exception.h"
#include "core/sysvirtualfile.h"
#include "core/cpcidskfile.h"
#include "segment/sysblockmap.h"
#include <cassert>
#include <cstring>
#if 0
#include <cstdio>
#endif

using namespace PCIDSK;


const int SysVirtualFile::block_size = SYSVIRTUALFILE_BLOCKSIZE;

/************************************************************************/
/*                           SysVirtualFile()                           */
/************************************************************************/

SysVirtualFile::SysVirtualFile( CPCIDSKFile *file, int start_block, 
                                uint64 image_length, 
                                PCIDSKBuffer &block_map_data,
                                SysBlockMap *sysblockmap,
                                int image_index )

{
    file_length = image_length;
    this->file = file;
    this->sysblockmap = sysblockmap;
    this->image_index = image_index;

    loaded_block = -1;
    loaded_block_dirty = false;
    
    last_bm_index = -1;

    int next_block = start_block;

    // perhaps we should defer all this work till the first request is made?
    while( next_block != -1 )
    {
        int offset = 512 + next_block * 28;

        block_segment.push_back( block_map_data.GetInt( offset+0, 4 ) );
        block_index.push_back( block_map_data.GetInt( offset+4, 8 ) );

        last_bm_index = next_block;
        next_block = block_map_data.GetInt( offset + 20, 8 );
    }

    assert( block_index.size() * block_size >= file_length );
}

/************************************************************************/
/*                          ~SysVirtualFile()                           */
/************************************************************************/

SysVirtualFile::~SysVirtualFile()

{
    Synchronize();
}

/************************************************************************/
/*                            Synchronize()                             */
/************************************************************************/

void SysVirtualFile::Synchronize()

{
    if( loaded_block_dirty )
    {
        PCIDSKSegment *data_seg_obj = 
            file->GetSegment( block_segment[loaded_block] );

        data_seg_obj->WriteToFile( block_data, 
                                   block_size * (uint64) block_index[loaded_block],
                                   block_size );
        loaded_block_dirty = false;
    }
}

/************************************************************************/
/*                            WriteToFile()                             */
/************************************************************************/

void 
SysVirtualFile::WriteToFile( const void *buffer, uint64 offset, uint64 size )

{
    uint64 buffer_offset = 0;

    while( buffer_offset < size )
    {
        int request_block = (int) ((offset + buffer_offset) / block_size);
        int offset_in_block = (int) ((offset + buffer_offset) % block_size);
        int amount_to_copy = block_size - offset_in_block;

        if (offset_in_block != 0 || amount_to_copy < block_size) {
            // we need to read in the block for update
            LoadBlock( request_block );
        } else {
            FlushDirtyBlock(); // flush the cached block, if dirty
            GrowVirtualFile(request_block); // if the file needs to grow, make it so
            loaded_block = request_block;
        }

        if( amount_to_copy > (int) (size - buffer_offset) )
            amount_to_copy = (int) (size - buffer_offset);

        // fill in the block
        memcpy( block_data + offset_in_block, 
                ((uint8 *) buffer) + buffer_offset, 
                amount_to_copy );

        loaded_block_dirty = true;

        buffer_offset += amount_to_copy;
    }

    if( offset+size > file_length )
    {
        file_length = offset+size;
        sysblockmap->SetVirtualFileSize( image_index, file_length );
    }
}

/************************************************************************/
/*                            ReadFromFile()                            */
/************************************************************************/

void SysVirtualFile::ReadFromFile( void *buffer, uint64 offset, uint64 size )

{
    uint64 buffer_offset = 0;
#if 0
    printf("Requesting region at %llu of size %llu\n", offset, size);
#endif
    while( buffer_offset < size )
    {
        int request_block = (int) ((offset + buffer_offset) / block_size);
        int offset_in_block = (int) ((offset + buffer_offset) % block_size);
        int amount_to_copy = block_size - offset_in_block;
        


        if (offset_in_block != 0 || (size - buffer_offset) < (uint64)block_size) {
            // Deal with the case where we need to load a partial block. Hopefully
            // this doesn't happen often
            LoadBlock( request_block );
            if( amount_to_copy > (int) (size - buffer_offset) )
                amount_to_copy = (int) (size - buffer_offset);
#if 0
            printf("Requested block: %d Offset: %d copying %d bytes\n",
                request_block, offset_in_block, amount_to_copy);
#endif
            memcpy( ((uint8 *) buffer) + buffer_offset,
                    block_data + offset_in_block, amount_to_copy );
        } else {
            // Use the bulk loading of blocks. First, compute the range
            // of full blocks we need to load
            int num_full_blocks = (size - buffer_offset)/block_size;
            
#if 0
            printf("Attempting a coalesced read of %d blocks (from %d) in buffer at %d\n", 
                num_full_blocks, request_block, buffer_offset);
#endif

            LoadBlocks(request_block, num_full_blocks, ((uint8*)buffer) + buffer_offset);
            amount_to_copy = num_full_blocks * block_size;
        }


        buffer_offset += amount_to_copy;
    }
}

/************************************************************************/
/*                             LoadBlock()                              */
/************************************************************************/
/**
 * Loads the requested_block block from the system virtual file. Extends
 * the file if necessary
 */
void SysVirtualFile::LoadBlock( int requested_block )

{
/* -------------------------------------------------------------------- */
/*      Do we already have this block?                                  */
/* -------------------------------------------------------------------- */
    if( requested_block == loaded_block )
        return;

/* -------------------------------------------------------------------- */
/*      Do we need to grow the virtual file by one block?               */
/* -------------------------------------------------------------------- */
    GrowVirtualFile(requested_block);

/* -------------------------------------------------------------------- */
/*      Does this block exist in the virtual file?                      */
/* -------------------------------------------------------------------- */
    if( requested_block < 0 || requested_block >= (int) block_index.size() )
        ThrowPCIDSKException( "SysVirtualFile::LoadBlock(%d) - block out of range.",
                              requested_block );

/* -------------------------------------------------------------------- */
/*      Do we have a dirty block loaded that needs to be saved?         */
/* -------------------------------------------------------------------- */
    FlushDirtyBlock();

/* -------------------------------------------------------------------- */
/*      Load the requested block.                                       */
/* -------------------------------------------------------------------- */
    PCIDSKSegment *data_seg_obj =
        file->GetSegment( block_segment[requested_block] );

    data_seg_obj->ReadFromFile( block_data,
                                block_size * (uint64) block_index[requested_block],
                                block_size );

    loaded_block = requested_block;
    loaded_block_dirty = false;
}

/************************************************************************/
/*                         FlushDirtyBlock()                            */
/************************************************************************/
/**
 * If the block currently loaded is dirty, flush it to the file
 */
void SysVirtualFile::FlushDirtyBlock(void)
{
    if (loaded_block_dirty) {
        PCIDSKSegment *data_seg_obj =
            file->GetSegment( block_segment[loaded_block] );
        
        data_seg_obj->WriteToFile( block_data,
                                   block_size * (uint64) block_index[loaded_block],
                                   block_size );
        loaded_block_dirty = false;
    }
}

void SysVirtualFile::GrowVirtualFile(std::ptrdiff_t requested_block)
{
    if( requested_block == (int) block_index.size() )
    {
        int new_seg;

        block_index.push_back( 
            sysblockmap->GrowVirtualFile( image_index,
                                          last_bm_index, new_seg ) );
        block_segment.push_back( new_seg );
    }
}

/**
 * \breif Load a group of blocks
 * Attempts to coalesce reading of groups of blocks into a single
 * filesystem I/O operation. Does not cache the loaded block, nor
 * does it modify the state of the SysVirtualFile, other than to
 * flush the loaded block if it is dirty.
 */
void SysVirtualFile::LoadBlocks(int requested_block_start,
                                int requested_block_count,
                                void* const buffer)
{
    FlushDirtyBlock();
    unsigned int blocks_read = 0;
    unsigned int current_start = requested_block_start;
    
    std::size_t buffer_off = 0;
    
    while (blocks_read < (unsigned int)requested_block_count) {
        // Coalesce blocks that are in the same segment
        unsigned int cur_segment = block_segment[current_start]; // segment of current
                // first block
        unsigned int cur_block = current_start; // starting block ID
        while (cur_block < (unsigned int)requested_block_count + requested_block_start &&
            (unsigned int)block_segment[cur_block + 1] == cur_segment) {
            // this block is in the same segment as the previous one we
            // wanted to read.
            cur_block++;
        }
        
        // now attempt to determine if the region of blocks (from current_start
        // to cur_block are contiguous
        uint64 read_start = block_index[current_start];
        uint64 read_cur = read_start * block_size;
        unsigned int count_to_read = 1; // we'll read at least one block
        while (read_cur + block_size ==
            (uint64)block_index[count_to_read + current_start] * block_size && // compare count of blocks * offset with stored offset
            count_to_read < (cur_block - current_start) ) // make sure we don't try to read more blocks than we determined fell in
                                                            // this segment
        {
            read_cur += block_size;
            count_to_read++;
        }

#if 0

        // Check if we need to grow the virtual file for each of these blocks
        for (unsigned int i = 0 ; i < count_to_read; i++) {
            GrowVirtualFile(i + current_start);
        }
        
        
        printf("Coalescing the read of %d blocks\n", count_to_read);
#endif

        // Perform the actual read
        PCIDSKSegment *data_seg_obj =
            file->GetSegment( cur_segment );
        
        std::size_t data_size = block_size * count_to_read;

#if 0
        printf("Reading %d bytes at offset %d in buffer\n", data_size, buffer_off);
#endif

        data_seg_obj->ReadFromFile( ((uint8*)buffer) + buffer_off,
                                    block_size * read_start,
                                    data_size );
                                    
        buffer_off += data_size; // increase buffer offset
        
        // Increment the current start by the number of blocks we jsut read
        current_start += count_to_read;
        blocks_read += count_to_read;
    }
}

