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
    FILE *fp = fopen( filename, access );

    if( fp == NULL )
        throw new PCIDSKException( "Failed to open %s: %s", 
                                   filename, LastError() );

    return (FILE *) fp;
}

/************************************************************************/
/*                                Seek()                                */
/************************************************************************/

uint64 
StdioIOInterface::Seek( void *io_handle, uint64 offset, int whence ) const

{
    uint64 result = fseek( (FILE *) io_handle, offset, whence );

    if( result == (uint64) -1 )
        throw new PCIDSKException( "Seek(%d,%d): %s", 
                                   (int) offset, whence, 
                                   LastError() );

    return result;
}

/************************************************************************/
/*                                Tell()                                */
/************************************************************************/

uint64 StdioIOInterface::Tell( void *io_handle ) const

{
    return ftell( (FILE *) io_handle );
}

/************************************************************************/
/*                                Read()                                */
/************************************************************************/

uint64 StdioIOInterface::Read( void *buffer, uint64 size, uint64 nmemb, 
                               void *io_handle ) const

{
    errno = 0;

    uint64 result = fread( buffer, size, nmemb, (FILE *) io_handle );

    if( errno != 0 && result == 0 && nmemb != 0 )
        throw new PCIDSKException( "Read(%d): %s", 
                                   (int) size * nmemb,
                                   LastError() );

    return result;
}

/************************************************************************/
/*                               Write()                                */
/************************************************************************/

uint64 StdioIOInterface::Write( const void *buffer, uint64 size, uint64 nmemb, 
                                void *io_handle ) const

{
    errno = 0;

    uint64 result = fwrite( buffer, size, nmemb, (FILE *) io_handle );

    if( errno != 0 && result == 0 && nmemb != 0 )
        throw new PCIDSKException( "Write(%d): %s", 
                                   (int) size * nmemb,
                                   LastError() );

    return result;
}

/************************************************************************/
/*                                Eof()                                 */
/************************************************************************/

int StdioIOInterface::Eof( void *io_handle ) const

{
    return feof( (FILE *) io_handle );
}

/************************************************************************/
/*                               Flush()                                */
/************************************************************************/

int StdioIOInterface::Flush( void *io_handle ) const

{
    return fflush( (FILE *) io_handle );
}

/************************************************************************/
/*                               Close()                                */
/************************************************************************/

int StdioIOInterface::Close( void *io_handle ) const

{
    return fclose( (FILE *) io_handle );
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
