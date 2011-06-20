/******************************************************************************
 *
 * Purpose:  Implementation of a buffered IO layer.
 * 
 ******************************************************************************
 * Copyright (c) 2011
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

#include "pcidsk_io.h"
#include "pcidsk_exception.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <cerrno>

using namespace PCIDSK;

class BufferedIOInterface : public IOInterfaces
{
public:
    virtual void   *Open( std::string filename, std::string access ) const;
    virtual uint64  Seek( void *io_handle, uint64 offset, int whence ) const;
    virtual uint64  Tell( void *io_handle ) const;
    virtual uint64  Read( void *buffer, uint64 size, uint64 nmemb, void *io_hanle ) const;
    virtual uint64  Write( const void *buffer, uint64 size, uint64 nmemb, void *io_handle ) const;
    virtual int     Eof( void *io_handle ) const;
    virtual int     Flush( void *io_handle ) const;
    virtual int     Close( void *io_handle ) const;

    const IOInterfaces   *base_io;
};

#define BUFFER_CHUNK_SIZE  8192

typedef struct {
    void         *base_io_handle;
    uint64        base_offset;    // what would tell() report on the base file?

    uint64        offset;         // last "tell()" location on our buffered file

    uint64        buffer_offset;  // where was buffer loaded from in file?
    uint64        buffer_used;    // how many bytes of buffer[] are filled?
    unsigned char buffer[BUFFER_CHUNK_SIZE];
} FileInfo;

/************************************************************************/
/*                      GetBufferedIOInterfaces()                       */
/************************************************************************/

const IOInterfaces *
PCIDSK::GetBufferedIOInterfaces( const IOInterfaces *base_io )

{
    // we really ought to create a new BufferedIOInterface for
    // each distinct base_io interface we given.
    static BufferedIOInterface singleton_buffered_interface;

    singleton_buffered_interface.base_io = base_io;

    return &singleton_buffered_interface;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

void *
BufferedIOInterface::Open( std::string filename, std::string access ) const

{
    void *base_io_handle = base_io->Open( filename, access );

    FileInfo *fi = new FileInfo();
    fi->base_io_handle = base_io_handle;
    fi->base_offset = 0;
    
    fi->buffer_offset = 0;
    fi->buffer_used = 0;

    fi->offset = 0;

    return fi;
}

/************************************************************************/
/*                                Seek()                                */
/************************************************************************/

uint64 
BufferedIOInterface::Seek( void *io_handle, uint64 offset, int whence ) const

{
    FileInfo *fi = (FileInfo *) io_handle;

    if( whence == SEEK_SET && offset == fi->offset )
        return 0;

    // If we are seeking within our existing buffer then we can short circuit
    // things. 
    if( whence == SEEK_SET 
        && offset >= fi->buffer_offset
        && offset <= fi->buffer_offset + fi->buffer_used )
    {
        fi->offset = offset;
        return 0;
    }

    // Otherwise we actually pass the seek down to the lower level.
    uint64 result = base_io->Seek( fi->base_io_handle, offset, whence );

    fi->base_offset = base_io->Tell( fi->base_io_handle );
    fi->offset = fi->base_offset;

    return result;
}

/************************************************************************/
/*                                Tell()                                */
/************************************************************************/

uint64 BufferedIOInterface::Tell( void *io_handle ) const

{
    FileInfo *fi = (FileInfo *) io_handle;

    return fi->offset;
}

/************************************************************************/
/*                                Read()                                */
/************************************************************************/

uint64 BufferedIOInterface::Read( void *buffer, uint64 size, uint64 nmemb, 
                               void *io_handle ) const

{
    FileInfo *fi = (FileInfo *) io_handle;
    uint64 bytes_to_read = size * nmemb;
    uint64 result;

/* -------------------------------------------------------------------- */
/*      The request can be completely satisfied from our existing       */
/*      loaded buffer.                                                  */
/* -------------------------------------------------------------------- */
    if( fi->offset >= fi->buffer_offset
        && fi->offset + bytes_to_read <= fi->buffer_offset + fi->buffer_used )
    {
        memcpy( buffer, fi->buffer + fi->offset - fi->buffer_offset, 
                bytes_to_read );
        fi->offset += bytes_to_read;
        return nmemb;
    }

/* -------------------------------------------------------------------- */
/*      Is this a pretty large request?  If so, just do a direct        */
/*      call.                                                           */
/* -------------------------------------------------------------------- */
    if( bytes_to_read > BUFFER_CHUNK_SIZE )
    {
        if( fi->offset != fi->base_offset )
        {
            base_io->Seek( fi->base_io_handle, fi->offset, SEEK_SET );
            fi->base_offset = fi->offset;
        }

        result = base_io->Read( buffer, size, nmemb, fi->base_io_handle );
        fi->base_offset += size * result;
        fi->offset += size * result;
        
        return result;
    }

/* -------------------------------------------------------------------- */
/*      We are going to load the buffer and satisfy the current         */
/*      request from it.                                                */
/*                                                                      */
/*      Eventually it would be nice to utilize any residual data in     */
/*      the old buffer that overlaps the request...                     */
/* -------------------------------------------------------------------- */
    if( fi->base_offset != fi->offset )
    {
        base_io->Seek( fi->base_io_handle, fi->offset, SEEK_SET );
        fi->base_offset = fi->offset;
    }

    fi->buffer_offset = fi->offset;
    fi->buffer_used = base_io->Read( fi->buffer, 1, BUFFER_CHUNK_SIZE, 
                                     fi->base_io_handle );
    fi->base_offset += fi->buffer_used;


/* -------------------------------------------------------------------- */
/*      Satisfy the request from the buffer as best possible.           */
/* -------------------------------------------------------------------- */
    if( fi->buffer_used >= size * nmemb )
        result = nmemb;
    else
        result = fi->buffer_used / size;

    memcpy( buffer, fi->buffer, result * size );

    fi->offset += result * size;

    return result;
}

/************************************************************************/
/*                               Write()                                */
/************************************************************************/

uint64 BufferedIOInterface::Write( const void *buffer, 
                                   uint64 size, uint64 nmemb, 
                                   void *io_handle ) const

{
    FileInfo *fi = (FileInfo *) io_handle;

    // disable read buffer.
    fi->buffer_used = 0;

    // Seek if needed.
    if( fi->base_offset != fi->offset )
    {
        base_io->Seek( fi->base_io_handle, fi->offset, SEEK_SET );
        fi->base_offset = fi->offset;
    }

    // Write.

    uint64 result = base_io->Write( buffer, size, nmemb, fi->base_io_handle );
    
    fi->offset += result * size;
    fi->base_offset += result * size;

    return result;
}

/************************************************************************/
/*                                Eof()                                 */
/************************************************************************/

int BufferedIOInterface::Eof( void *io_handle ) const

{
    FileInfo *fi = (FileInfo *) io_handle;

    // Seek if needed.
    if( fi->base_offset != fi->offset )
    {
        base_io->Seek( fi->base_io_handle, fi->offset, SEEK_SET );
        fi->base_offset = fi->offset;
    }

    return base_io->Eof( fi->base_io_handle );
}

/************************************************************************/
/*                               Flush()                                */
/************************************************************************/

int BufferedIOInterface::Flush( void *io_handle ) const

{
    FileInfo *fi = (FileInfo *) io_handle;
    return base_io->Flush( fi->base_io_handle );
}

/************************************************************************/
/*                               Close()                                */
/************************************************************************/

int BufferedIOInterface::Close( void *io_handle ) const

{
    FileInfo *fi = (FileInfo *) io_handle;

    int result = base_io->Close( fi->base_io_handle );

    delete fi;

    return result;
}
