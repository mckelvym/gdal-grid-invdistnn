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
/*                          PCIDSK_CopyBits()                           */
/*                                                                      */
/*      Copy bit strings - adapted from GDAL.                           */
/************************************************************************/

static void 
PCIDSK_CopyBits( const uint8 *pabySrcData, int nSrcOffset, int nSrcStep, 
                 uint8 *pabyDstData, int nDstOffset, int nDstStep,
                 int nBitCount, int nStepCount )

{
    int iStep;
    int iBit;

    for( iStep = 0; iStep < nStepCount; iStep++ )
    {
        for( iBit = 0; iBit < nBitCount; iBit++ )
        {
            if( pabySrcData[nSrcOffset>>3] 
                & (0x80 >>(nSrcOffset & 7)) )
                pabyDstData[nDstOffset>>3] |= (0x80 >> (nDstOffset & 7));
            else
                pabyDstData[nDstOffset>>3] &= ~(0x80 >> (nDstOffset & 7));


            nSrcOffset++;
            nDstOffset++;
        } 

        nSrcOffset += (nSrcStep - nBitCount);
        nDstOffset += (nDstStep - nBitCount);
    }
}

/************************************************************************/
/*                             ReadBlock()                              */
/************************************************************************/

int CPCIDSKBitmap::ReadBlock( int block_index, void *buffer,
                              int win_xoff, int win_yoff,
                              int win_xsize, int win_ysize )

{
    uint64 block_size = (block_width * block_height + 7) / 8;
    uint8 *wrk_buffer = (uint8 *) buffer;

    if( block_index < 0 || block_index >= GetBlockCount() )
    {
        ThrowPCIDSKException( "Requested non-existant block (%d)", 
                              block_index );
    }
/* -------------------------------------------------------------------- */
/*      If we are doing subwindowing, we will need to create a          */
/*      temporary bitmap to load into.  If we are concerned about       */
/*      high performance access to small windows in big bitmaps we      */
/*      will eventually want to reimplement this to avoid reading       */
/*      the whole block to subwindow from.                              */
/* -------------------------------------------------------------------- */
    if( win_ysize != -1 )
    {
        if( win_xoff < 0 || win_xoff + win_xsize > GetBlockWidth()
            || win_yoff < 0 || win_yoff + win_ysize > GetBlockHeight() )
        {
            ThrowPCIDSKException( 
                "Invalid window in CPCIDSKBitmap::ReadBlock(): xoff=%d,yoff=%d,xsize=%d,ysize=%d",
                win_xoff, win_yoff, win_xsize, win_ysize );
        }

        wrk_buffer = (uint8 *) malloc(block_size);
        if( wrk_buffer == NULL )
            ThrowPCIDSKException( "Out of memory allocating %d bytes in CPCIDSKBitmap::ReadBlock()", 
                                  (int) block_size );
    }

/* -------------------------------------------------------------------- */
/*      Read the block, taking care in the case of partial blocks at    */
/*      the bottom of the image.                                        */
/* -------------------------------------------------------------------- */
    if( (block_index+1) * block_height <= height )
        ReadFromFile( wrk_buffer, block_size * block_index, block_size );
    else
    {
        uint64 short_block_size;
        
        memset( buffer, 0, block_size );
        
        short_block_size = 
            ((height - block_index*block_height) * block_width + 7) / 8;
        
        ReadFromFile( wrk_buffer, block_size * block_index, short_block_size );
    }

/* -------------------------------------------------------------------- */
/*      Perform subwindowing if needed.                                 */
/* -------------------------------------------------------------------- */
    if( win_ysize != -1 )
    {
        int y_out;

        for( y_out = 0; y_out <  win_ysize; y_out++ )
        {
            PCIDSK_CopyBits( wrk_buffer, 
                             win_xoff + (y_out+win_yoff)*block_width, 0, 
                             (uint8*) buffer, y_out * win_xsize, 0, 
                             win_xsize, 1 );
        }

        free( wrk_buffer );
    }

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

std::string CPCIDSKBitmap::GetMetadataValue( const std::string &key )

{
    return CPCIDSKSegment::GetMetadataValue( key );
}

/************************************************************************/
/*                          SetMetadataValue()                          */
/************************************************************************/

void CPCIDSKBitmap::SetMetadataValue( const std::string &key, 
                                      const std::string &value )

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

void CPCIDSKBitmap::SetDescription( const std::string &description )

{
    CPCIDSKSegment::SetDescription( description );
}

/************************************************************************/
/*                         GetHistoryEntries()                          */
/************************************************************************/

std::vector<std::string> CPCIDSKBitmap::GetHistoryEntries() const

{
    return CPCIDSKSegment::GetHistoryEntries();
}

/************************************************************************/
/*                         SetHistoryEntries()                          */
/************************************************************************/

void CPCIDSKBitmap::SetHistoryEntries( const std::vector<std::string> &entries )

{
    CPCIDSKSegment::SetHistoryEntries( entries );
}

/************************************************************************/
/*                            PushHistory()                             */
/************************************************************************/

void CPCIDSKBitmap::PushHistory( const std::string &app, 
                                 const std::string &message )

{
    CPCIDSKSegment::PushHistory( app, message );
}
                                 

