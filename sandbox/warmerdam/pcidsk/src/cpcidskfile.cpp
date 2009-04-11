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
    std::string interleaving = fh.Get(360,8);

    uint64 image_offset = (image_start_block-1) * 512;

/* -------------------------------------------------------------------- */
/*      Initialize the list of channels.                                */
/* -------------------------------------------------------------------- */
    int channelnum;

    for( channelnum = 1; channelnum <= channel_count; channelnum++ )
    {
        PCIDSKBuffer ih(1024);
        PCIDSKChannel *channel;
        
            ReadFromFile( ih.buffer, 
                          (ih_start_block-1)*512 + (channelnum-1)*1024, 
                          1024);
        
        if( interleaving == "BAND    " )
        {
            channel = new CBandInterleavedChannel( ih, fh, channelnum, this,
                                                   image_offset );

            
            image_offset += DataTypeSize(channel->GetType())
                * width * height;
            
            // bands start on block boundaries I think.
            image_offset = ((image_offset+511) / 512) * 512;
                                       
        }

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


