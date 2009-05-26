/******************************************************************************
 *
 * Purpose:  Implementation of the Open() function.
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

/**
 * Open a PCIDSK (.pix) file. 
 *
 * This function attempts to open the named file, with the indicated
 * access and the provided set of system interface methods.
 *
 * @param filename the name of the PCIDSK file to access.
 * @param access either "r" for read-only, or "r+" for read-write access.
 * @param interfaces Either NULL to use default interfaces, or a pointer
 * to a populated interfaces object. 
 *
 * @return a pointer to a file object for accessing the PCIDSK file. 
 */

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

	if ( interfaces->io->Read( header_check, 1, 6, io_handle ) != 6 )
	{
		interfaces->io->Close( io_handle );
		throw new PCIDSKException( "File %s is smaller than the PCIDSK header magic.",
				filename );
	}

    if( !memcmp(header_check,"PCIDSK",6) )
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
