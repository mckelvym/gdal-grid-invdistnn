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

using namespace PCIDSK;

class StdioIOInterface : public IOInterfaces
{
    virtual void   *Open( const char *filename, const char *access );
    virtual uint64  Seek( void *io_handle, uint64 offset, int whence );
    virtual uint64  Tell( void *io_handle );
    virtual uint64  Read( void *buffer, uint64 size, uint64 nmemb, void *io_handle );
    virtual uint64  Write( void *buffer, uint64 size, uint64 nmemb, void *io_handle );
    virtual int     Eof( void *io_handle );
    virtual int     Flush( void *io_handle );
    virtual int     Close( void *io_handle );
};

/************************************************************************/
/*                       GetDefaultIOInterfaces()                       */
/************************************************************************/

const IOInterfaces *GetDefaultIOInterfaces()
{
    static StdioIOInterface singleton_stdio_interface;

    return &singleton_stdio_interface;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

void *StdioIOInterface::Open( const char *filename, const char *access )

{
    return (void *) fopen( filename, access );
}

/************************************************************************/
/*                                Seek()                                */
/************************************************************************/

uint64 StdioIOInterface::Seek( void *io_handle, uint64 offset, int whence )

{
    return fseek( (FILE *) io_handle, offset, whence );
}

/************************************************************************/
/*                                Tell()                                */
/************************************************************************/

uint64 StdioIOInterface::Tell( void *io_handle )

{
    return ftell( (FILE *) io_handle );
}

/************************************************************************/
/*                                Read()                                */
/************************************************************************/

uint64 StdioIOInterface::Read( void *buffer, uint64 size, uint64 nmemb, 
                               void *io_handle )

{
    return fread( buffer, size, nmemb, (FILE *) io_handle );
}

/************************************************************************/
/*                               Write()                                */
/************************************************************************/

uint64 StdioIOInterface::Write( void *buffer, uint64 size, uint64 nmemb, 
                                void *io_handle )

{
    return fwrite( buffer, size, nmemb, (FILE *) io_handle );
}

/************************************************************************/
/*                                Eof()                                 */
/************************************************************************/

int StdioIOInterface::Eof( void *io_handle )

{
    return feof( (FILE *) io_handle );
}

/************************************************************************/
/*                               Flush()                                */
/************************************************************************/

int StdioIOInterface::Flush( void *io_handle )

{
    return fflush( (FILE *) io_handle );
}

/************************************************************************/
/*                               Close()                                */
/************************************************************************/

int StdioIOInterface::Close( void *io_handle )

{
    return fclose( (FILE *) io_handle );
}
