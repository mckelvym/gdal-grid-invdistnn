/******************************************************************************
 *
 * Purpose:  Implementation of the CPCIDSKBitmap class.
 * 
 ******************************************************************************
 * Copyright (c) 2010
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

#include "pcidsk_exception.h"
#include "segment/cpcidskbitmap.h"
#include "pcidsk_file.h"
#include "core/pcidsk_utils.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>

using namespace PCIDSK;

/************************************************************************/
/*                           CPCIDSKBitmap()                            */
/************************************************************************/

CPCIDSKBitmap::CPCIDSKBitmap( PCIDSKFile *file, int segment,
                              const char *segment_pointer )
        : CPCIDSKSegment( file, segment, segment_pointer )

{
    loaded = false;
}

/************************************************************************/
/*                           ~CPCIDSKBitmap()                           */
/************************************************************************/

CPCIDSKBitmap::~CPCIDSKBitmap()

{
}

/************************************************************************/
/*                             Initialize()                             */
/*                                                                      */
/*      Set up a newly created bitmap segment.  We just need to         */
/*      write some stuff into the segment header.                       */
/************************************************************************/

void CPCIDSKBitmap::Initialize()

{
    loaded = false;

    CPCIDSKBitmap *pThis = (CPCIDSKBitmap *) this;

    PCIDSKBuffer &bheader = pThis->GetHeader();

    bheader.Put( 0, 160     , 16 );
    bheader.Put( 0, 160+16*1, 16 );
    bheader.Put( file->GetWidth(), 160+16*2, 16 );
    bheader.Put( file->GetHeight(), 160+16*3, 16 );
    bheader.Put( -1, 160+16*4, 16 );

    file->WriteToFile( bheader.buffer, data_offset, 1024 );
}

/************************************************************************/
/*                                Load()                                */
/************************************************************************/

void CPCIDSKBitmap::Load() const

{
    if( loaded )
        return;

    // We don't really mean the internals are const, just a lie to 
    // keep the const interfaces happy.

    CPCIDSKBitmap *pThis = (CPCIDSKBitmap *) this;

    PCIDSKBuffer &bheader = pThis->GetHeader();

    pThis->width  = bheader.GetInt( 192,    16 );
    pThis->height = bheader.GetInt( 192+16, 16 );

    // Choosing 8 lines per block ensures that each block 
    // starts on a byte boundary.
    pThis->block_width = pThis->width;
    pThis->block_height = 8;

    pThis->loaded = true;
}

/************************************************************************/
/*                           GetBlockWidth()                            */
/************************************************************************/

int CPCIDSKBitmap::GetBlockWidth() const

{
    if( !loaded )
        Load();

    return block_width;
}

/************************************************************************/
/*                           GetBlockHeight()                           */
/************************************************************************/

int CPCIDSKBitmap::GetBlockHeight() const

{
    if( !loaded )
        Load();

    return block_height;
}

/************************************************************************/
/*                           GetBlockCount()                            */
/************************************************************************/

int CPCIDSKBitmap::GetBlockCount() const

{
    if( !loaded )
        Load();

    return ((width + block_width - 1) / block_width)
        * ((height + block_height - 1) / block_height);
}

/************************************************************************/
/*                              GetWidth()                              */
/************************************************************************/

int CPCIDSKBitmap::GetWidth() const

{
    if( !loaded )
        Load();

    return width;
}

/************************************************************************/
/*                             GetHeight()                              */
/************************************************************************/

int CPCIDSKBitmap::GetHeight() const

{
    if( !loaded )
        Load();

    return height;
}

/************************************************************************/
/*                              GetType()                               */
/************************************************************************/

eChanType CPCIDSKBitmap::GetType() const

{
    return CHN_BIT;
}

/************************************************************************/
/*                             ReadBlock()                              */
/************************************************************************/

int CPCIDSKBitmap::ReadBlock( int block_index, void *buffer,
                              int win_xoff, int win_yoff,
                              int win_xsize, int win_ysize )

{
    uint64 block_size = (block_width * block_height) / 8;

    if( win_xoff == -1 && win_yoff == -1
        && win_xsize == -1 && win_ysize == -1 )
    {
        if( (block_index+1) * block_height <= height )
            ReadFromFile( buffer, block_size * block_index, block_size );
        else
        {
            uint64 short_block_size;

            memset( buffer, 0, block_size );

            short_block_size = 
                ((height - block_index*block_height) * block_width + 7) / 8;

            ReadFromFile( buffer, block_size * block_index, short_block_size );
        }
        return 1;
    }

    // TODO
    ThrowPCIDSKException("Windowing on bitmap blocks not yet supported." );

    return 0;
}

/************************************************************************/
/*                             WriteBlock()                             */
/************************************************************************/

int CPCIDSKBitmap::WriteBlock( int block_index, void *buffer )

{
    uint64 block_size = (block_width * block_height) / 8;

    if( (block_index+1) * block_height <= height )
        WriteToFile( buffer, block_size * block_index, block_size );
    else
    {
        uint64 short_block_size;
        
        short_block_size = 
            ((height - block_index*block_height) * block_width + 7) / 8;
        
        WriteToFile( buffer, block_size * block_index, short_block_size );
    }

    return 1;
}

/************************************************************************/
/*                          GetOverviewCount()                          */
/************************************************************************/

int CPCIDSKBitmap::GetOverviewCount()

{
    return 0;
}

/************************************************************************/
/*                            GetOverview()                             */
/************************************************************************/

PCIDSKChannel *CPCIDSKBitmap::GetOverview( int i )

{
    ThrowPCIDSKException("Non-existant overview %d requested on bitmap segment."); 
    return NULL;
}

/************************************************************************/
/*                          GetMetadataValue()                          */
/************************************************************************/

std::string CPCIDSKBitmap::GetMetadataValue( std::string key )

{
    return CPCIDSKSegment::GetMetadataValue( key );
}

/************************************************************************/
/*                          SetMetadataValue()                          */
/************************************************************************/

void CPCIDSKBitmap::SetMetadataValue( std::string key, std::string value )

{
    CPCIDSKSegment::SetMetadataValue( key, value );
}

/************************************************************************/
/*                          GetMetadataKeys()                           */
/************************************************************************/

std::vector<std::string> CPCIDSKBitmap::GetMetadataKeys()

{
    return CPCIDSKSegment::GetMetadataKeys();
}

/************************************************************************/
/*                            Synchronize()                             */
/************************************************************************/

void CPCIDSKBitmap::Synchronize()

{
    // TODO 

    CPCIDSKSegment::Synchronize();
}

/************************************************************************/
/*                           GetDescription()                           */
/************************************************************************/

std::string CPCIDSKBitmap::GetDescription()

{
    return CPCIDSKSegment::GetDescription();
}

/************************************************************************/
/*                           SetDescription()                           */
/************************************************************************/

void CPCIDSKBitmap::SetDescription( std::string description )

{
// TODO    CPCIDSKSegment::SetDescription( description );
}

