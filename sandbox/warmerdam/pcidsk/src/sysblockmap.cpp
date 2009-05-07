/******************************************************************************
 *
 * Purpose:  Implementation of the SysBlockMap class.
 *
 * This class is used to manage access to the SYS virtual block map segment
 * (named SysBMap).  This segment is used to keep track of one or more 
 * virtual files stored in SysBData segments.  These virtual files are normally
 * used to hold tiled images for primary bands or overviews.  
 *
 * This class is closely partnered with the SysVirtualFile class, and the
 * primary client is the CTiledChannel class. 
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
/*                            SysBlockMap()                             */
/************************************************************************/

SysBlockMap::SysBlockMap( CPCIDSKFile *file, int segment,
                              const char *segment_pointer )
        : CPCIDSKSegment( file, segment, segment_pointer )

{
    loaded = false;
}

/************************************************************************/
/*                            ~SysBlockMap()                            */
/************************************************************************/

SysBlockMap::~SysBlockMap()

{
    size_t i;
    
    for( i = 0; i < virtual_files.size(); i++ )
    {
        delete virtual_files[i];
        virtual_files[i] = NULL;
    }
}

/************************************************************************/
/*                                Load()                                */
/************************************************************************/

void SysBlockMap::Load()

{
    if( loaded )
        return;

    // TODO: this should likely be protected by a mutex. 

/* -------------------------------------------------------------------- */
/*      Load the segment contents into a buffer.                        */
/* -------------------------------------------------------------------- */
    seg_data.SetSize( (int) (data_size - 1024) );

    ReadFromFile( seg_data.buffer, 0, data_size - 1024 );

    if( strncmp(seg_data.buffer,"VERSION",7) != 0 )
        throw new PCIDSKException( "SysBlockMap::Load() - block map corrupt." );

/* -------------------------------------------------------------------- */
/*      Establish our SysVirtualFile array based on the number of       */
/*      images listed in the image list.                                */
/* -------------------------------------------------------------------- */
    int layer_count = seg_data.GetInt( 10, 8 );

    block_count = seg_data.GetInt( 18, 8 );
    first_free_block = seg_data.GetInt( 26, 8 );

    virtual_files.resize( layer_count );

    block_map_offset = 512;
    layer_list_offset = block_map_offset + 28 * block_count;
}

/************************************************************************/
/*                          GetImageSysFile()                           */
/************************************************************************/

SysVirtualFile *SysBlockMap::GetImageSysFile( int image )

{
    Load();

    if( image < 0 || image >= (int) virtual_files.size() )
        throw new PCIDSKException( "GetImageSysFile(%d): invalid image index",
                                   image );

    if( virtual_files[image] != NULL )
        return virtual_files[image];

    uint64  vfile_length = 
        seg_data.GetUInt64( layer_list_offset + 24*image + 12, 12 );
    int  start_block = 
        seg_data.GetInt( layer_list_offset + 24*image + 4, 8 );

    virtual_files[image] = 
        new SysVirtualFile( file, start_block, vfile_length, seg_data );

    return virtual_files[image];
}
