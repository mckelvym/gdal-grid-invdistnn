/******************************************************************************
 *
 * Purpose:  Implementation of the CPCIDSKFile class.
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
/*                                Open()                                */
/************************************************************************/

PCIDSKFile *PCIDSK::Open( const char *filename, const char *access,
                          const PCIDSKInterfaces *interfaces )

{
/* -------------------------------------------------------------------- */
/*      Use default interfaces if none are passed in.                   */
/* -------------------------------------------------------------------- */
    PCIDSKInterfaces default_interfaces;
    if( interfaces == NULL )
        interfaces = &default_interfaces;

/* -------------------------------------------------------------------- */
/*      First open the file, and confirm that it is PCIDSK before       */
/*      going further.                                                  */
/* -------------------------------------------------------------------- */
    void *io_handle = interfaces->io->Open( filename, access );

    assert( io_handle != NULL );

    char header_check[6];

    if( interfaces->io->Read( header_check, 1, 6, io_handle ) != 6 
        || memcmp(header_check,"PCIDSK",6) != 0 )
    {
        interfaces->io->Close( io_handle );
        throw new PCIDSKException( "File %s does not appear to be PCIDSK format.",
                                   filename );
    }

/* -------------------------------------------------------------------- */
/*      Create the PCIDSKFile object.                                   */
/* -------------------------------------------------------------------- */

    CPCIDSKFile *file = new CPCIDSKFile();
    
    file->interfaces = *interfaces;
    file->io_handle = io_handle;
    file->io_mutex = interfaces->CreateMutex();

/* -------------------------------------------------------------------- */
/*      Initialize it from the header.                                  */
/* -------------------------------------------------------------------- */
    try
    {
        file->InitializeFromHeader();
    }
    catch(...)
    {
        delete file;
        throw;
    }

    return file;
}

/************************************************************************/
/*                             CPCIDSKFile()                             */
/************************************************************************/

CPCIDSKFile::CPCIDSKFile()

{
    io_handle = NULL;
    io_mutex = NULL;
}

/************************************************************************/
/*                            ~CPCIDSKFile()                             */
/************************************************************************/

CPCIDSKFile::~CPCIDSKFile()

{
/* -------------------------------------------------------------------- */
/*      Cleanup last line caching stuff for pixel interleaved data.     */
/* -------------------------------------------------------------------- */
    if( last_block_data != NULL )
    {
        FlushBlock();
        last_block_index = -1;
        free( last_block_data );
        last_block_data = NULL;
        delete last_block_mutex;
    }

    
/* -------------------------------------------------------------------- */
/*      Close and cleanup IO stuff.                                     */
/* -------------------------------------------------------------------- */
    {
        MutexHolder oHolder( io_mutex );

        if( io_handle )
        {
            interfaces.io->Close( io_handle );
            io_handle = NULL;
        }
    }

    delete io_mutex;
}

/************************************************************************/
/*                             GetChannel()                             */
/************************************************************************/

PCIDSKChannel *CPCIDSKFile::GetChannel( int band )

{
    if( band < 1 || band > channel_count )
        throw PCIDSKException( "Out of range band (%d) requested.", 
                               band );

    return channels[band-1];
}

/************************************************************************/
/*                             GetSegment()                             */
/************************************************************************/

PCIDSK::PCIDSKObject *CPCIDSKFile::GetSegment( int segment )

{
    throw new PCIDSKException( "Segment access not implemented yet." );
}

/************************************************************************/
/*                             GetObjects()                             */
/************************************************************************/

std::vector<PCIDSK::PCIDSKObject *> CPCIDSKFile::GetObjects()

{
    throw new PCIDSKException( "Objects list access not implemented yet." );
}

/************************************************************************/
/*                        InitializeFromHeader()                        */
/************************************************************************/

void CPCIDSKFile::InitializeFromHeader()

{
/* -------------------------------------------------------------------- */
/*      Process the file header.                                        */
/* -------------------------------------------------------------------- */
    PCIDSKBuffer fh(512);

    ReadFromFile( fh.buffer, 0, 512 );

    width = atoi(fh.Get(384,8));
    height = atoi(fh.Get(392,8));
    channel_count = atoi(fh.Get(376,8));

    uint64 ih_start_block = atouint64(fh.Get(336,16));
    uint64 image_start_block = atouint64(fh.Get(304,16));
    fh.Get(360,8,interleaving);

    uint64 image_offset = (image_start_block-1) * 512;

    block_size = 0;
    last_block_index = -1;
    last_block_dirty = 0;
    last_block_data = NULL;
    last_block_mutex = NULL;

/* -------------------------------------------------------------------- */
/*      Load the segment pointers into a PCIDSKBuffer.  For now we      */
/*      try to avoid doing too much other processing on them.           */
/* -------------------------------------------------------------------- */
    int segment_block_count = atoi(fh.Get(456,8));
    
    segment_count = (segment_block_count * 512) / 32;
    segment_pointers.SetSize( segment_block_count * 512 );
    ReadFromFile( segment_pointers.buffer, 
                  atouint64(fh.Get(440,16)) * 512, 
                  segment_block_count * 512 );

/* -------------------------------------------------------------------- */
/*      Get the number of each channel type - only used for some        */
/*      interleaving cases.                                             */
/* -------------------------------------------------------------------- */
    int count_8u = atoi(fh.Get(464,4));
    int count_16s = atoi(fh.Get(468,4));
    int count_16u = atoi(fh.Get(472,4));
    int count_32r = atoi(fh.Get(476,4));

/* -------------------------------------------------------------------- */
/*      for pixel interleaved files we need to compute the length of    */
/*      a scanline padded out to a 512 byte boundary.                   */
/* -------------------------------------------------------------------- */
    if( interleaving == "PIXEL" )
    {
        first_line_offset = image_offset;
        pixel_group_size = count_8u + count_16s*2 + count_16u*2 + count_32r*4;
        
        block_size = pixel_group_size * width;
        if( block_size % 512 != 0 )
            block_size += 512 - (block_size % 512);

        last_block_data = malloc(block_size);
        if( last_block_data == NULL )
            throw new PCIDSKException( "Allocating %d bytes for scanline buffer failed.", 
                                       (int) block_size );

        last_block_mutex = interfaces.CreateMutex();
        image_offset = 0;
    }

/* -------------------------------------------------------------------- */
/*      Initialize the list of channels.                                */
/* -------------------------------------------------------------------- */
    int channelnum;

    for( channelnum = 1; channelnum <= channel_count; channelnum++ )
    {
        PCIDSKBuffer ih(1024);
        PCIDSKChannel *channel = NULL;
        
        ReadFromFile( ih.buffer, 
                      (ih_start_block-1)*512 + (channelnum-1)*1024, 
                      1024);

        // fetch the filename, if there is one.
        std::string filename;
        ih.Get(64,64,filename);

        // work out channel type from header
        eChanType pixel_type;
        const char *pixel_type_string = ih.Get( 160, 8 );
    
        if( strcmp(pixel_type_string,"8U      ") == 0 )
            pixel_type = CHN_8U;
        else if( strcmp(pixel_type_string,"16S     ") == 0 )
            pixel_type = CHN_16S;
        else if( strcmp(pixel_type_string,"16U     ") == 0 )
            pixel_type = CHN_16U;
        else if( strcmp(pixel_type_string,"32R     ") == 0 )
            pixel_type = CHN_32R;
        else
            pixel_type = CHN_UNKNOWN; // should we throw an exception?  

        // if we didn't get channel type in header, work out from counts (old)

        if( channelnum <= count_8u )
            pixel_type = CHN_8U;
        else if( channelnum <= count_8u + count_16s )
            pixel_type = CHN_16S;
        else if( channelnum <= count_8u + count_16s + count_16u )
            pixel_type = CHN_16U;
        else 
            pixel_type = CHN_32R;
            
        if( interleaving == "BAND" )
        {
            channel = new CBandInterleavedChannel( ih, fh, channelnum, this,
                                                   image_offset, pixel_type );

            
            image_offset += DataTypeSize(channel->GetType())
                * width * height;
            
            // bands start on block boundaries I think.
            image_offset = ((image_offset+511) / 512) * 512;
        }

        else if( interleaving == "PIXEL" )
        {
            channel = new CPixelInterleavedChannel( ih, fh, channelnum, this,
                                                    (int) image_offset, 
                                                    pixel_type );
            image_offset += DataTypeSize(pixel_type);
        }

        else if( interleaving == "FILE" 
                 && strncmp(filename.c_str(),"/SIS=",5) == 0 )
        {
        }

        else if( interleaving == "FILE" )
        {
            channel = new CBandInterleavedChannel( ih, fh, channelnum, this,
                                                   0, pixel_type );
        }

        else
            throw new PCIDSKException( "Unsupported interleaving:%s", 
                                       interleaving.c_str() );

        channels.push_back( channel );
    }
}

/************************************************************************/
/*                            ReadFromFile()                            */
/************************************************************************/

void CPCIDSKFile::ReadFromFile( void *buffer, uint64 offset, uint64 size )

{
    MutexHolder oHolder( io_mutex );

    interfaces.io->Seek( io_handle, offset, SEEK_SET );
    if( interfaces.io->Read( buffer, 1, size, io_handle ) != size )
        throw new PCIDSKException( "PCIDSKFile:Failed to read %d bytes at %d.", 
                                   (int) size, (int) offset );
}

/************************************************************************/
/*                            WriteToFile()                             */
/************************************************************************/

void CPCIDSKFile::WriteToFile( const void *buffer, uint64 offset, uint64 size )

{
    MutexHolder oHolder( io_mutex );

    interfaces.io->Seek( io_handle, offset, SEEK_SET );
    if( interfaces.io->Write( buffer, 1, size, io_handle ) != size )
        throw new PCIDSKException( "PCIDSKFile:Failed to write %d bytes at %d.",
                                   (int) size, (int) offset );
}

/************************************************************************/
/*                          ReadAndLockBlock()                          */
/************************************************************************/

void *CPCIDSKFile::ReadAndLockBlock( int block_index )

{
    if( last_block_data == NULL )
        throw new PCIDSKException( "ReadAndLockBlock() called on a file that is not pixel interleaved." );

    if( block_index == last_block_index )
    {
        last_block_mutex->Acquire();
        return last_block_data;
    }

    FlushBlock();

    last_block_mutex->Acquire();

    ReadFromFile( last_block_data, 
                  first_line_offset + block_index*block_size,
                  block_size );
    last_block_index = block_index;
    
    return last_block_data;
}

/************************************************************************/
/*                            UnlockBlock()                             */
/************************************************************************/

void CPCIDSKFile::UnlockBlock( int mark_dirty )

{
    if( last_block_mutex == NULL )
        return;

    last_block_dirty |= mark_dirty;
    last_block_mutex->Release();
}

/************************************************************************/
/*                             WriteBlock()                             */
/************************************************************************/

void CPCIDSKFile::WriteBlock( int block_index, void *buffer )

{
    if( last_block_data == NULL )
        throw new PCIDSKException( "WriteBlock() called on a file that is not pixel interleaved." );

    WriteToFile( buffer,
                 first_line_offset + block_index*block_size,
                 block_size );
}

/************************************************************************/
/*                             FlushBlock()                             */
/************************************************************************/

void CPCIDSKFile::FlushBlock()

{
    if( last_block_dirty ) 
    {
        last_block_mutex->Acquire();
        if( last_block_dirty ) // is it still dirty?
        {
            WriteBlock( last_block_index, last_block_data );
            last_block_dirty = 0;
        }
        last_block_mutex->Release();
    }
}


