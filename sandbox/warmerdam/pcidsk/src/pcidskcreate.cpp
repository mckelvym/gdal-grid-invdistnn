/******************************************************************************
 *
 * Purpose:  Implementation of the Create() function to create new PCIDSK files.
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
 * @param channel_count the number of channels to create.
 * @param channel_types an array of types for all the channels, or NULL for
 * all CHN_8U channels.
 * @param option creation options (interleaving, etc)
 * @param interfaces Either NULL to use default interfaces, or a pointer
 * to a populated interfaces object. 
 *
 * @return a pointer to a file object for accessing the PCIDSK file. 
 */

PCIDSKFile PCIDSK_DLL *
PCIDSK::Create( const char *filename, int pixels, int lines,
                int channel_count, eChanType *channel_types,
                const char *options, const PCIDSKInterfaces *interfaces )

{
/* -------------------------------------------------------------------- */
/*      Use default interfaces if none are passed in.                   */
/* -------------------------------------------------------------------- */
    PCIDSKInterfaces default_interfaces;
    if( interfaces == NULL )
        interfaces = &default_interfaces;

/* -------------------------------------------------------------------- */
/*      Default the channel types to all 8U if not provided.            */
/* -------------------------------------------------------------------- */
    std::vector<eChanType> default_channel_types;

    if( channel_types == NULL )
    {
        default_channel_types.resize( channel_count, CHN_8U );
        channel_types = &(default_channel_types[0]);
    }
   
/* -------------------------------------------------------------------- */
/*      Validate parameters.                                            */
/* -------------------------------------------------------------------- */
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
        ThrowPCIDSKException( "PCIDSK::Create() options '%s' not recognised.", 
                              options );

    if( strstr(options,"NOZERO") != NULL )
        nozero = true;

/* -------------------------------------------------------------------- */
/*      Validate the channel types.                                     */
/* -------------------------------------------------------------------- */
    int channels[4] = {0,0,0,0};
    int chan_index;
    bool regular = true;

    for( chan_index=0; chan_index < channel_count; chan_index++ )
    {
        if( chan_index > 0 
            && ((int) channel_types[chan_index]) 
                < ((int) channel_types[chan_index-1]) )
            regular = false;
        
        channels[((int) channel_types[chan_index])]++;
    }
    
    if( !regular && strcmp(interleaving,"FILE") != 0 )
    {
        ThrowPCIDSKException( 
           "Requested mixture of band types not supported for interleaving=%s.",
           interleaving );
    }
    
/* -------------------------------------------------------------------- */
/*      Create the file.                                                */
/* -------------------------------------------------------------------- */
    void *io_handle = interfaces->io->Open( filename, "w+" );

    assert( io_handle != NULL );

/* ==================================================================== */
/*      Establish some key file layout information.                     */
/* ==================================================================== */
    int image_header_start = 2, image_header_size; // in blocks
    uint64 image_data_start, image_data_size;      // in blocks
    uint64 segment_ptr_start, segment_ptr_size=64; // in blocks
    int pixel_group_size, line_size;               // in bytes

/* -------------------------------------------------------------------- */
/*      Pixel interleaved.                                              */
/* -------------------------------------------------------------------- */
    if( strcmp(interleaving,"PIXEL") == 0 )
    {
        image_header_size = channel_count * 2;
        pixel_group_size = 
            channels[0] + channels[1]*2 + channels[2]*2 + channels[3]*4;
        line_size = ((pixel_group_size * pixels + 511) / 512) * 512;
        image_data_size = (((uint64)line_size) * lines) / 512;

        // TODO: Old code enforces a 1TB limit for some reason.
    }

/* -------------------------------------------------------------------- */
/*      Band interleaved.                                               */
/* -------------------------------------------------------------------- */
    else if( strcmp(interleaving,"BAND") == 0 )
    {
        image_header_size = channel_count * 2;

        pixel_group_size = 
            channels[0] + channels[1]*2 + channels[2]*2 + channels[3]*4;
        // BAND interleaved bands are tightly packed.
        image_data_size = 
            (((uint64)pixel_group_size) * pixels * lines + 511) / 512;

        // TODO: Old code enforces a 1TB limit for some reason.
    }

/* -------------------------------------------------------------------- */
/*      FILE/Tiled.                                                     */
/* -------------------------------------------------------------------- */
    else if( strcmp(interleaving,"FILE") == 0 )
    {
        // For some reason we reserve extra space, but only for FILE.
        if( channel_count < 64 )
            image_header_size = 64 * 2;
        else
            image_header_size = channel_count * 2;

        image_data_size = 0;

        // TODO: Old code enforces a 1TB limit on the fattest band.
    }
    
/* -------------------------------------------------------------------- */
/*      Place components.                                               */
/* -------------------------------------------------------------------- */
    segment_ptr_start = image_header_start + image_header_size;
    image_data_start = segment_ptr_start + segment_ptr_size;

/* ==================================================================== */
/*      Prepare the file header.                                        */
/* ==================================================================== */
    PCIDSKBuffer fh(512);
    static const char default_time[] = "15:13 08May2009 ";

    // Initialize everything to spaces.
    fh.Put( "", 0, 512 );

/* -------------------------------------------------------------------- */
/*      File Type, Version, and Size                                    */
/* 	Notice: we get the first 4 characters from PCIVERSIONAME.	*/
/* -------------------------------------------------------------------- */
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
    fh.Put( default_time, 272, 16 );

    // FH9 Update date/time
    fh.Put( default_time, 288, 16 );

/* -------------------------------------------------------------------- */
/*      Image Data                                                      */
/* -------------------------------------------------------------------- */
    // FH10 - start block of image data
    fh.Put( image_data_start+1, 304, 16 );

    // FH11 - number of blocks of image data.
    fh.Put( image_data_size, 320, 16 );

    // FH12 - start block of image headers.
    fh.Put( image_header_start+1, 336, 16 );

    // FH13 - number of blocks of image headers.
    fh.Put( image_header_size, 352, 8);

    // FH14 - interleaving.
    fh.Put( interleaving, 360, 8);

    // FH15 - reserved - MIXED is for some ancient backwards compatability.
    fh.Put( "MIXED", 368, 8);

    // FH16 - number of image bands.
    fh.Put( channel_count, 376, 8 );

    // FH17 - width of image in pixels.
    fh.Put( pixels, 384, 8 );

    // FH18 - height of image in pixels.
    fh.Put( lines, 392, 8 );

    // FH19 - pixel ground size interpretation.
    fh.Put( "METRE", 400, 8 );
    
    // TODO:
    //PrintDouble( fh->XPixelSize, "%16.9f", 1.0 );
    //PrintDouble( fh->YPixelSize, "%16.9f", 1.0 );
    fh.Put( "1.0", 408, 16 );
    fh.Put( "1.0", 424, 16 );

/* -------------------------------------------------------------------- */
/*      Segment Pointers                                                */
/* -------------------------------------------------------------------- */
    // FH22 - start block of segment pointers.
    fh.Put( segment_ptr_start+1, 440, 16 );

    // fH23 - number of blocks of segment pointers.
    fh.Put( segment_ptr_size, 456, 8 );

/* -------------------------------------------------------------------- */
/*      Number of different types of Channels                           */
/* -------------------------------------------------------------------- */
    // FH24.1 - 8U bands.
    fh.Put( channels[0], 464, 4 );

    // FH24.2 - 16S bands.
    fh.Put( channels[1], 468, 4 );

    // FH24.3 - 16U bands.
    fh.Put( channels[2], 472, 4 );

    // FH24.4 - 32R bands.
    fh.Put( channels[3], 476, 4 );

/* -------------------------------------------------------------------- */
/*      Write out the file header.                                      */
/* -------------------------------------------------------------------- */
    interfaces->io->Write( fh.buffer, 512, 1, io_handle );

/* ==================================================================== */
/*      Write out the image headers.                                    */
/* ==================================================================== */
    PCIDSKBuffer ih( 1024 );

    ih.Put( " ", 0, 1024 );

    // IHi.1 - Text describing Channel Contents
    ih.Put( "Contents Not Specified", 0, 64 );

    // IHi.2 - Filename storing image.
    if( strcmp(interleaving,"FILE") == 0 )
        ih.Put( "<unintialized>", 64, 64 );
    
    // IHi.3 - Creation time and date.
    ih.Put( default_time, 128, 16 );

    // IHi.4 - Creation time and date.
    ih.Put( default_time, 144, 16 );

    interfaces->io->Seek( io_handle, image_header_start*512, SEEK_SET );

    for( chan_index = 0; chan_index < channel_count; chan_index++ )
    {
        if( channel_types[chan_index] == CHN_8U )
            ih.Put( "8U", 160, 8 );
        else if( channel_types[chan_index] == CHN_16S )
            ih.Put( "16S", 160, 8 );
        else if( channel_types[chan_index] == CHN_16U )
            ih.Put( "16U", 160, 8 );
        else if( channel_types[chan_index] == CHN_32R )
            ih.Put( "32R", 160, 8 );

        interfaces->io->Write( ih.buffer, 1024, 1, io_handle );
    }

/* ==================================================================== */
/*      Write out the segment pointers, all spaces.                     */
/* ==================================================================== */
    PCIDSKBuffer segment_pointers( segment_ptr_size*512 );
    segment_pointers.Put( " ", 0, segment_ptr_size*512 );

    interfaces->io->Seek( io_handle, segment_ptr_start*512, SEEK_SET );
    interfaces->io->Write( segment_pointers.buffer, segment_ptr_size, 512, 
                           io_handle );

/* -------------------------------------------------------------------- */
/*      Ensure we write out something at the end of the image data      */
/*      to force the file size.                                         */
/* -------------------------------------------------------------------- */
    interfaces->io->Seek( io_handle, (image_data_start + image_data_size)*512-1,
                          SEEK_SET );
    interfaces->io->Write( "\0", 1, 1, io_handle );
    
/* -------------------------------------------------------------------- */
/*      Close the raw file, and reopen as a pcidsk file.                */
/* -------------------------------------------------------------------- */
    interfaces->io->Close( io_handle );
    
    return Open( filename, "r+", interfaces );
}
