/******************************************************************************
 *
 * Purpose:  Implementation of a stdio based IO layer.
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

#include "pcidsk.h"
#include <stdio.h>
#include <errno.h>

using namespace PCIDSK;

class StdioIOInterface : public IOInterfaces
{
    virtual void   *Open( const char *filename, const char *access ) const;
    virtual uint64  Seek( void *io_handle, uint64 offset, int whence ) const;
    virtual uint64  Tell( void *io_handle ) const;
    virtual uint64  Read( void *buffer, uint64 size, uint64 nmemb, void *io_handle ) const;
    virtual uint64  Write( const void *buffer, uint64 size, uint64 nmemb, void *io_handle ) const;
    virtual int     Eof( void *io_handle ) const;
    virtual int     Flush( void *io_handle ) const;
    virtual int     Close( void *io_handle ) const;

    const char     *LastError() const;
};

typedef struct {
    FILE   *fp;
    uint64 offset;
} FileInfo;

/************************************************************************/
/*                       GetDefaultIOInterfaces()                       */
/************************************************************************/

const IOInterfaces *PCIDSK::GetDefaultIOInterfaces()
{
    static StdioIOInterface singleton_stdio_interface;

    return &singleton_stdio_interface;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

void *
StdioIOInterface::Open( const char *filename, const char *access ) const

{
    std::string adjusted_access = access;

    adjusted_access += "b";

    FILE *fp = fopen( filename, adjusted_access.c_str() );

    if( fp == NULL )
        throw new PCIDSKException( "Failed to open %s: %s", 
                                   filename, LastError() );

    FileInfo *fi = new FileInfo();
    fi->fp = fp;
    fi->offset = 0;

    return fi;
}

/************************************************************************/
/*                                Seek()                                */
/************************************************************************/

uint64 
StdioIOInterface::Seek( void *io_handle, uint64 offset, int whence ) const

{
    FileInfo *fi = (FileInfo *) io_handle;

    // seeks that do nothing are still surprisingly expensive with MSVCRT.
    // try and short circuit if possible.
    if( whence == SEEK_SET && offset == fi->offset )
        return 0;

    uint64 result = fseek( fi->fp, offset, whence );

    if( result == (uint64) -1 )
        throw new PCIDSKException( "Seek(%d,%d): %s", 
                                   (int) offset, whence, 
                                   LastError() );

    if( whence == SEEK_SET )
        fi->offset = offset;
    else if( whence == SEEK_END )
        fi->offset = ftell( fi->fp );
    else if( whence == SEEK_CUR )
        fi->offset += offset;

    return result;
}

/************************************************************************/
/*                                Tell()                                */
/************************************************************************/

uint64 StdioIOInterface::Tell( void *io_handle ) const

{
    FileInfo *fi = (FileInfo *) io_handle;

    return ftell( fi->fp );
}

/************************************************************************/
/*                                Read()                                */
/************************************************************************/

uint64 StdioIOInterface::Read( void *buffer, uint64 size, uint64 nmemb, 
                               void *io_handle ) const

{
    FileInfo *fi = (FileInfo *) io_handle;

    errno = 0;

    uint64 result = fread( buffer, size, nmemb, fi->fp );

    if( errno != 0 && result == 0 && nmemb != 0 )
        throw new PCIDSKException( "Read(%d): %s", 
                                   (int) size * nmemb,
                                   LastError() );

    fi->offset += size*result;

    return result;
}

/************************************************************************/
/*                               Write()                                */
/************************************************************************/

uint64 StdioIOInterface::Write( const void *buffer, uint64 size, uint64 nmemb, 
                                void *io_handle ) const

{
    FileInfo *fi = (FileInfo *) io_handle;

    errno = 0;

    uint64 result = fwrite( buffer, size, nmemb, fi->fp );

    if( errno != 0 && result == 0 && nmemb != 0 )
        throw new PCIDSKException( "Write(%d): %s", 
                                   (int) size * nmemb,
                                   LastError() );

    fi->offset += size*result;

    return result;
}

/************************************************************************/
/*                                Eof()                                 */
/************************************************************************/

int StdioIOInterface::Eof( void *io_handle ) const

{
    FileInfo *fi = (FileInfo *) io_handle;
    return feof( fi->fp );
}

/************************************************************************/
/*                               Flush()                                */
/************************************************************************/

int StdioIOInterface::Flush( void *io_handle ) const

{
    FileInfo *fi = (FileInfo *) io_handle;
    return fflush( fi->fp );
}

/************************************************************************/
/*                               Close()                                */
/************************************************************************/

int StdioIOInterface::Close( void *io_handle ) const

{
    FileInfo *fi = (FileInfo *) io_handle;
    int result = fclose( fi->fp );

    delete fi;

    return result;
}

/************************************************************************/
/*                             LastError()                              */
/*                                                                      */
/*      Return a string representation of the last error.               */
/************************************************************************/

const char *StdioIOInterface::LastError() const

{
    return strerror( errno );
}
