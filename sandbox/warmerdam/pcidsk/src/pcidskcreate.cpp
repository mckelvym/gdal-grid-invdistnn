/******************************************************************************
 *
 * Purpose:  Implementation of the Create() function.
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
/*                               Create()                               */
/************************************************************************/

/**
 * Create a PCIDSK (.pix) file. 
 *
 * @param filename the name of the PCIDSK file to create.
 * @param pixel the width of the new file in pixels.
 * @param lines the height of the new file in scanlines.
 * @param channels the number of channels to create of each channel type.
 * @param option creation options (interleaving, etc)
 * @param interfaces Either NULL to use default interfaces, or a pointer
 * to a populated interfaces object. 
 *
 * @return a pointer to a file object for accessing the PCIDSK file. 
 */

PCIDSKFile PCIDSK_DLL *Create( const char *filename, int pixels, int lines,
                               int *channels, const char *options,
                               const PCIDSKInterfaces *interfaces )

{
/* -------------------------------------------------------------------- */
/*      Use default interfaces if none are passed in.                   */
/* -------------------------------------------------------------------- */
    PCIDSKInterfaces default_interfaces;
    if( interfaces == NULL )
        interfaces = &default_interfaces;

/* -------------------------------------------------------------------- */
/*      Validate parameters.                                            */
/* -------------------------------------------------------------------- */
    int channel_count = channels[0] + channels[1] + channels[2] + channels[3];
    const char *interleaving;
    bool nozero = false;

    if(strncmp(options,"PIXEL",5) == 0 )
        interleaving = "PIXEL";
    else if( strncmp(options,"BAND",4) == 0 )
        interleaving = "BAND";
    else if( strncmp(options,"TILED",5) == 0 
             || strncmp(options,"FILE",4) == 0 )
        interleaving = "FILE";
    else
        throw new 
            PCIDSKException( "PCIDSK::Create() options '%s' not recognised.", 
                             options );

    if( strstr(options,"NOZERO") != NULL )
        no_zero = true;
    
/* -------------------------------------------------------------------- */
/*      Create the file.                                                */
/* -------------------------------------------------------------------- */
    void *io_handle = interfaces->io->Open( filename, "w+" );

    assert( io_handle != NULL );

/* -------------------------------------------------------------------- */
/*      File Type, Version, and Size                                    */
/* 	Notice: we get the first 4 characters from PCIVERSIONAME.	*/
/* -------------------------------------------------------------------- */
    PCIDSKBuffer fh(512);

    // Initialize everything to spaces.
    fh.Put( "", 0, 512 );

    // FH1 - magic format string.
    fh.Put( "PCIDSK", 0, 8 );

    // FH2 - TODO: Allow caller to pass this in.
    fh.Put( "SDK V1.0", 8, 8 );

    // FH3 - TODO: fill in file size later.
    fh.Put( "", 16, 16 );
    
    // FH4 - 16 characters reserved - spaces.

    // FH5 - Description
    fh.Put( filename, 48, 64 );

    // FH6 - Facility
    fh.Put( "PCI Inc., Richmond Hill, Canada", 112, 32 );

    // FH7.1 / FH7.2 - left blank (64+64 bytes @ 144)

    // FH8 Creation date/time
    fh.Put( "", 272, 16 );

    // FH9 Update date/time
    fh.Put( "", 288, 16 );

/* -------------------------------------------------------------------- */
/*      Image Data                                                      */
/* -------------------------------------------------------------------- */
    PrintInt( fh->ImageStart, "%16ld", ImgDataSt );
    PrintInt( fh->ImageSize, "%16ld", ImgDataSz );
    PrintInt( fh->ImageHeadStart, "%16ld", ImgHeadSt );
    PrintInt( fh->ImageHeadSize, "%8ld", ImgHeadSz );

    if( EQUAL(pszInterleaving,"pixel") )
        TextFieldFill( fh->Interleave, "PIXEL", 8 );
    else if( EQUAL(pszInterleaving,"band") ) 
        TextFieldFill( fh->Interleave, "BAND", 8 );
    else
        TextFieldFill( fh->Interleave, "FILE", 8 );

    PrintInt( fh->Channels, "%8d", tchan );
    PrintInt( fh->Pixels, "%8d", pixels );
    PrintInt( fh->Lines, "%8d", lines );

    TextFieldFill( fh->PixelUnit, "METRE", 8 );
    PrintDouble( fh->XPixelSize, "%16.9f", 1.0 );
    PrintDouble( fh->YPixelSize, "%16.9f", 1.0 );

/* -------------------------------------------------------------------- */
/*      Segment Pointers                                                */
/* -------------------------------------------------------------------- */
    sprintf( buffer, "%16ld%8ld", SegPtrSt, SegPtrSz );
    strncpy( pcibuf+440, buffer, 24 );
	
    PrintInt( fh->SegmentHeadStart, "%16ld", SegPtrSt );
    PrintInt( fh->SegmentHeadSize, "%8ld", SegPtrSz );

/* -------------------------------------------------------------------- */
/*      Number of different types of Channels                           */
/* -------------------------------------------------------------------- */
    PrintInt( fh->Num8U, "%4d", channels[0] );
    PrintInt( fh->Num16S, "%4d", channels[1] );
    PrintInt( fh->Num16U, "%4d", channels[2] );
    PrintInt( fh->Num32R, "%4d", channels[3] );

/* -------------------------------------------------------------------- */
/*      For backward compatiblity we fill in field FH15                 */
/* -------------------------------------------------------------------- */
    TextFieldFill( fh->Reserved2, "MIXED", 8 );

/* -------------------------------------------------------------------- */
/*              Write the master header block (block 1)                 */
/* -------------------------------------------------------------------- */
    DKPut( idb_fp, 1L, 1L, pcibuf );

/* -------------------------------------------------------------------- */
/*      Clear Buffer to Blanks                                          */
/* -------------------------------------------------------------------- */
    memset(pcibuf, ' ', 1024);

    return file;
}
