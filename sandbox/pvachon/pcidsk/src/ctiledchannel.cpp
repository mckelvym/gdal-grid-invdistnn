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
                              CPCIDSKFile *file,
                              eChanType pixel_type )
        : CPCIDSKChannel( image_header, file, pixel_type, channelnum )

{
/* -------------------------------------------------------------------- */
/*      Establish the virtual file we will be accessing.                */
/* -------------------------------------------------------------------- */
    std::string filename;

    image_header.Get(64,64,filename);

    assert( strstr(filename.c_str(),"SIS=") != NULL );

    image = atoi(strstr(filename.c_str(),"SIS=") + 4);

    vfile = NULL;

/* -------------------------------------------------------------------- */
/*      If this is an unassociated channel (ie. an overview), we        */
/*      will set the size and blocksize values to someone               */
/*      unreasonable and set them properly in EstablishAccess()         */
/* -------------------------------------------------------------------- */
    if( channelnum == -1 )
    {
        width = -1;
        height = -1;
        block_width = -1;
        block_height = -1;
    }
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
    
    if( data_type == "8U" )
        pixel_type = CHN_8U;
    else if( data_type == "16S" )
        pixel_type = CHN_16S;
    else if( data_type == "16U" )
        pixel_type = CHN_16U;
    else if( data_type == "32R" )
        pixel_type = CHN_32R;
    else
    {
        throw new PCIDSKException( "Unknown channel type: %s", 
                                   data_type.c_str() );
    }

/* -------------------------------------------------------------------- */
/*      Extract the tile map                                            */
/* -------------------------------------------------------------------- */
    int tiles_per_row = (width + block_width - 1) / block_width;
    int tiles_per_col = (height + block_height - 1) / block_height;
    int tile_count = tiles_per_row * tiles_per_col;
    int i;

    tile_offsets.resize( tile_count );
    tile_sizes.resize( tile_count );

    PCIDSKBuffer tmap( tile_count * 20 );

    vfile->ReadFromFile( tmap.buffer, 128, tile_count*20 );
    
    for( i = 0; i < tile_count; i++ )
    {
        tile_offsets[i] = tmap.GetUInt64( i*12 + 0, 12 );
        tile_sizes[i] = tmap.GetInt( tile_count*12 + i*8, 8 );
    }

/* -------------------------------------------------------------------- */
/*      Establish byte swapping.  Tiled data files are always big       */
/*      endian, regardless of what the headers might imply.             */
/* -------------------------------------------------------------------- */
    unsigned short test_value = 1;
    
    if( ((uint8 *) &test_value)[0] == 1 )
        needs_swap = pixel_type != CHN_8U;
    else
        needs_swap = false;
}

/************************************************************************/
/*                             ReadBlock()                              */
/************************************************************************/

int CTiledChannel::ReadBlock( int block_index, void *buffer,
                              int xoff, int yoff, 
                              int xsize, int ysize )

{
    if( !vfile )
        EstablishAccess();

    int pixel_size = DataTypeSize(GetType());

/* -------------------------------------------------------------------- */
/*      Default window if needed.                                       */
/* -------------------------------------------------------------------- */
    if( xoff == -1 && yoff == -1 && xsize == -1 && ysize == -1 )
    {
        xoff = 0;
        yoff = 0;
        xsize = GetBlockWidth();
        ysize = GetBlockHeight();
    }

/* -------------------------------------------------------------------- */
/*      Validate Window                                                 */
/* -------------------------------------------------------------------- */
    if( xoff < 0 || xoff + xsize > GetBlockWidth()
        || yoff < 0 || yoff + ysize > GetBlockHeight() )
    {
        throw new PCIDSKException( 
            "Invalid window in ReadBloc(): xoff=%d,yoff=%d,xsize=%d,ysize=%d",
            xoff, yoff, xsize, ysize );
    }

    if( block_index < 0 || block_index >= (int) tile_offsets.size() )
    {
        throw new PCIDSKException( "Requested non-existant block (%d)", 
                                   block_index );
    }

/* -------------------------------------------------------------------- */
/*      The simpliest case it an uncompressed direct and complete       */
/*      tile read into the destination buffer.                          */
/* -------------------------------------------------------------------- */
    if( xoff == 0 && xsize == GetBlockWidth() 
        && yoff == 0 && ysize == GetBlockHeight() 
        && compression == "NONE" )
    {
        vfile->ReadFromFile( buffer, 
                             tile_offsets[block_index], 
                             tile_sizes[block_index] );
        // Do byte swapping if needed.
        if( needs_swap )
            SwapData( buffer, pixel_size, xsize * ysize );

        return 1;
    }

/* -------------------------------------------------------------------- */
/*      For now we do not support compression.                          */
/* -------------------------------------------------------------------- */
    if( compression != "NONE" )
    {
        throw new PCIDSKException( "Compression type '%s' is not currently supported.", compression.c_str() );
    }

/* -------------------------------------------------------------------- */
/*      Load uncompressed data, one scanline at a time, into the        */
/*      target buffer.                                                  */
/* -------------------------------------------------------------------- */
    int iy;

    for( iy = 0; iy < ysize; iy++ )
    {
        vfile->ReadFromFile( ((uint8 *) buffer) 
                             + iy * xsize * pixel_size,
                             tile_offsets[block_index] 
                             + ((iy+yoff)*block_width + xoff) * pixel_size,
                             xsize * pixel_size );
    }

    // Do byte swapping if needed.
    if( needs_swap )
        SwapData( buffer, pixel_size, xsize * ysize );

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

/************************************************************************/
/*                              GetWidth()                              */
/************************************************************************/

int CTiledChannel::GetWidth()

{
    if( width == -1 )
        EstablishAccess();

    return CPCIDSKChannel::GetWidth();
}

/************************************************************************/
/*                             GetHeight()                              */
/************************************************************************/

int CTiledChannel::GetHeight()

{
    if( height == -1 )
        EstablishAccess();

    return CPCIDSKChannel::GetHeight();
}

/************************************************************************/
/*                              GetType()                               */
/************************************************************************/

eChanType CTiledChannel::GetType()

{
    if( pixel_type == CHN_UNKNOWN )
        EstablishAccess();

    return CPCIDSKChannel::GetType();
}

