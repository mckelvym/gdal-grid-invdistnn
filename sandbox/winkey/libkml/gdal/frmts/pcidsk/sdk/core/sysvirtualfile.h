/******************************************************************************
 *
 * Purpose:  Declaration of the SysVirtualFile class.
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
#ifndef __INCLUDE_CORE_SYSVIRTUALFILE_H
#define __INCLUDE_CORE_SYSVIRTUALFILE_H

#include "pcidsk_buffer.h"

#include <vector>

#define SYSVIRTUALFILE_BLOCKSIZE    8192

namespace PCIDSK
{
    class CPCIDSKFile;
    class SysBlockMap;
    /************************************************************************/
    /*                            SysVirtualFile                            */
    /************************************************************************/

    class SysVirtualFile
    {
    public:
        SysVirtualFile( CPCIDSKFile *file, int start_block, uint64 image_length,
                        PCIDSKBuffer &block_map_data, SysBlockMap *sysblockmap,
                        int image_index );
        ~SysVirtualFile();

        void      Synchronize();

        void      WriteToFile( const void *buffer, uint64 offset, uint64 size );
        void      ReadFromFile( void *buffer, uint64 offset, uint64 size );

        uint64    GetLength() { return file_length; }
    
        static const int       block_size;
    
    private:
        CPCIDSKFile           *file;
        SysBlockMap           *sysblockmap;
        int                    image_index;

        uint64                 file_length;

        std::vector<int>       block_segment;
        std::vector<int>       block_index;

        int                    loaded_block;
        uint8                  block_data[SYSVIRTUALFILE_BLOCKSIZE];
        bool                   loaded_block_dirty;

        int                    last_bm_index;

        void                   LoadBlock( int requested_block );
        void                   LoadBlocks( int requested_block_start,
            int requested_block_count, void* const buffer);
        void                   GrowVirtualFile(std::ptrdiff_t requested_block);
        void                   FlushDirtyBlock();
        void                   WriteBlocks(int first_block, int block_count,
            void* const buffer);
    };
}

#endif // __INCLUDE_CORE_SYSVIRTUALFILE_H
