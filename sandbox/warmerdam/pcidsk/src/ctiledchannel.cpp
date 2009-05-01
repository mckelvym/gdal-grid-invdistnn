/******************************************************************************
 *
 * Purpose:  Implementation of the CTiledChannel class.
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
/*                      CTiledChannel()                                 */
/************************************************************************/

CTiledChannel::CTiledChannel( PCIDSKBuffer &image_header, 
                              PCIDSKBuffer &file_header,
                              int channelnum,
                              CPCIDSKFile *file )
        : CPCIDSKChannel( image_header, file, pixel_type )

{
/* -------------------------------------------------------------------- */
/*      Establish the virtual file we will be accessing.                */
/* -------------------------------------------------------------------- */
    std::string filename;

    image_header.Get(64,64,filename);

    assert( strstr(filename.c_str(),"SIS=") != NULL );

    image = atoi(strstr(filename.c_str(),"SIS=") + 4);

    vfile = NULL;
}

/************************************************************************/
/*                           ~CTiledChannel()                           */
/************************************************************************/

CTiledChannel::~CTiledChannel()

{
}

/************************************************************************/
/*                          EstablishAccess()                           */
/************************************************************************/

void CTiledChannel::EstablishAccess()

{
    if( vfile != NULL )
        return;
    
/* -------------------------------------------------------------------- */
/*      Establish the virtual file to access this image.                */
/* -------------------------------------------------------------------- */
    SysBlockMap *bmap = dynamic_cast<SysBlockMap*>(
        file->GetSegment( SEG_SYS, "SysBMDir" ));

    if( bmap == NULL )
        throw new PCIDSKException( "Unable to find SysBMap segmet." );

    vfile = bmap->GetImageSysFile( image );

/* -------------------------------------------------------------------- */
/*      Parse the header.                                               */
/* -------------------------------------------------------------------- */
    PCIDSKBuffer theader(128);
    std::string data_type;

    vfile->ReadFromFile( theader.buffer, 0, 128 );

    width = theader.GetInt(0,8);
    height = theader.GetInt(8,8);
    block_width = theader.GetInt(16,8);
    block_height = theader.GetInt(24,8);

    theader.Get(32,4,data_type);
    theader.Get(54, 8, compression);
    
    // TODO: translate data type. 

/* -------------------------------------------------------------------- */
/*      Extract the tile map                                            */
/* -------------------------------------------------------------------- */
    int tiles_per_row = (width + block_width - 1) / block_width;
    int tiles_per_col = (height + block_height - 1) / block_height;
    uint64 tile_count = tiles_per_row * tiles_per_col;
    uint64 i;

    tile_offsets.resize( tile_count );
    tile_sizes.resize( tile_count );

    PCIDSKBuffer tmap( tile_count * 20 );

    vfile->ReadFromFile( tmap.buffer, 128, tile_count*20 );
    
    for( i = 0; i < tile_count; i++ )
    {
        tile_offsets[i] = tmap.GetUInt64( i*12 + 0, 12 );
        tile_sizes[i] = tmap.GetInt( tile_count*12 + i*8, 8 );
    }
}

/************************************************************************/
/*                             ReadBlock()                              */
/************************************************************************/

int CTiledChannel::ReadBlock( int block_index, void *buffer )

{
    if( !vfile )
        EstablishAccess();

/* -------------------------------------------------------------------- */
/*      Fetch the desired block.                                        */
/* -------------------------------------------------------------------- */
    vfile->ReadFromFile( buffer, 
                         tile_offsets[block_index], 
                         tile_sizes[block_index] );

/* -------------------------------------------------------------------- */
/*      Do byte swapping if needed.                                     */
/* -------------------------------------------------------------------- */
    int pixel_size = DataTypeSize(GetType());

    if( needs_swap )
        SwapData( buffer, pixel_size, width );

    return 1;
}

/************************************************************************/
/*                             WriteBlock()                             */
/************************************************************************/

int CTiledChannel::WriteBlock( int block_index, void *buffer )

{
    throw new PCIDSKException( "WriteBlock not implemented yet." );
}

/************************************************************************/
/*                           GetBlockWidth()                            */
/************************************************************************/

int CTiledChannel::GetBlockWidth()

{
    EstablishAccess();
    return CPCIDSKChannel::GetBlockWidth();
}

/************************************************************************/
/*                           GetBlockHeight()                           */
/************************************************************************/

int CTiledChannel::GetBlockHeight()

{
    EstablishAccess();
    return CPCIDSKChannel::GetBlockHeight();
}
